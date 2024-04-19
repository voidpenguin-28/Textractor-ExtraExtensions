#pragma once

#include "Libraries/inihandler.h"
#include "Libraries/stringconvert.h"
#include "HtmlParser.h"
#include "HttpClient.h"
#include "ExtensionConfig.h"
#include <functional>
#include <string>
using namespace std;


class NameRetriever {
public:
	virtual ~NameRetriever() { }
	virtual wstring_map_pair getNameMappings(const string& vnId) = 0;
};


class NoNameRetriever : public NameRetriever {
public:
	wstring_map_pair getNameMappings(const string& vnId) override {
		return wstring_map_pair();
	}
};


class VndbHttpNameRetriever : public NameRetriever {
public:
	VndbHttpNameRetriever(const function<string()> urlTemplateGetter, 
		const HttpClient& httpClient, const HtmlParser& htmlParser)
		: _urlTemplateGetter(urlTemplateGetter), _httpClient(httpClient), _htmlParser(htmlParser) { }

	VndbHttpNameRetriever(const string& urlTemplate, 
		const HttpClient& httpClient, const HtmlParser& htmlParser)
		: VndbHttpNameRetriever([urlTemplate]() { return urlTemplate; }, httpClient, htmlParser) { }

	wstring_map_pair getNameMappings(const string& vnId) override {
		if (vnId.empty()) return wstring_map_pair();
		string url = createUrl(vnId);
		string html = getAllCharsHtmlPage(url);

		wstring_map_pair nameMap = _htmlParser.getNameMappings(html);
		return nameMap;
	}
private:
	const vector<string> _httpHeaders = { "Cookie: vndb_samesite=1" };
	const function<string()> _urlTemplateGetter;
	const HttpClient& _httpClient;
	const HtmlParser& _htmlParser;

	string createUrl(string vnId) const {
		static const string tmplPar = "{0}";
		if (vnId[0] != 'v') vnId = 'v' + vnId;

		string urlTemplate = _urlTemplateGetter();
		size_t tmplIndex = urlTemplate.find(tmplPar);
		if (tmplIndex == string::npos)
			runtime_error("UrlTemplate config value is invalid. Must contain '{0}' value to indicate expected location of vn_id (ex: https://vndb.org/{0}/chars). Value: " + urlTemplate);

		return urlTemplate.replace(tmplIndex, tmplPar.length(), vnId);
	}

	string getAllCharsHtmlPage(string baseUrl) const {
		string html = _httpClient.httpGet(baseUrl);
		string spoilCharsPath = _htmlParser.extractSpoilCharsPath(html);
		html = _httpClient.httpGet(baseUrl + spoilCharsPath, _httpHeaders);
		return html;
	}
};


class CacheNameRetriever : public NameRetriever {
public:
	CacheNameRetriever(NameRetriever& mainRetriever, const function<bool()> reloadCacheGetter) 
		: _mainRetriever(mainRetriever), _reloadCacheGetter(reloadCacheGetter) { }

	wstring_map_pair getNameMappings(const string& vnId) override {
		if (vnId.empty()) return wstring_map_pair();
		lock_guard<mutex> lock(_mutex);
		bool reloadCache = _reloadCacheGetter();
		wstring_map_pair map;

		if (!reloadCache) {
			map = getMapFromCache(vnId);
			if (!map.first.empty()) return map;
		}

		map = _mainRetriever.getNameMappings(vnId);
		saveMapToCache(vnId, map);
		return map;
	}
protected:
	NameRetriever& _mainRetriever;
	const function<bool()> _reloadCacheGetter;
	mutable mutex _mutex;

	virtual wstring_map_pair getMapFromCache(const string& vnId) const = 0;
	virtual void saveMapToCache(const string& vnId, const wstring_map_pair& map) = 0;
};


class IniFileCacheNameRetriever : public CacheNameRetriever {
public:
	IniFileCacheNameRetriever(const string& iniFileName, NameRetriever& mainNameRetriever, 
		const function<bool()> reloadCacheGetter) : CacheNameRetriever(mainNameRetriever, reloadCacheGetter), 
			_iniFileName(iniFileName), _iniHandler(iniFileName) { }
protected:
	const string _iniFileName;
	const IniFileHandler _iniHandler;

	wstring_map_pair getMapFromCache(const string& vnId) const override {
		auto ini = unique_ptr<IniContents>(_iniHandler.readIni());

		wstring fullNameSection = createFullNameSectionName(vnId);
		wstring_map fullNameMap = getNamesFromSection(*ini, fullNameSection);

		wstring singleNameSection = createSingleNameSectionName(vnId);
		wstring_map singleNameMap = getNamesFromSection(*ini, singleNameSection);
		return wstring_map_pair(fullNameMap, singleNameMap);
	}

	void saveMapToCache(const string& vnId, const wstring_map_pair& map) override {
		auto ini = unique_ptr<IniContents>(_iniHandler.readIni());
		
		wstring fullNameSection = createFullNameSectionName(vnId);
		addNamesToSection(*ini, fullNameSection, map.first);

		wstring singleNameSection = createSingleNameSectionName(vnId);
		addNamesToSection(*ini, singleNameSection, map.second);

		_iniHandler.saveIni(*ini);
	}
private:
	wstring createFullNameSectionName(const string& vnId) const {
		return convertToW(vnId) + L"-Full";
	}

	wstring createSingleNameSectionName(const string& vnId) const {
		return convertToW(vnId) + L"-Single";
	}

	wstring_map getNamesFromSection(const IniContents& ini, const wstring& sectionName) const {
		vector<pair<wstring, wstring>> fullNamesList = ini.getAllValues(sectionName);
		wstring_map nameMap = pairVectorToMap(fullNamesList);
		return nameMap;
	}

	wstring_map pairVectorToMap(vector<pair<wstring, wstring>> pairVector) const {
		wstring_map map{};

		for (const auto& p : pairVector) {
			if (p.first.empty()) continue;
			map[p.first] = p.second;
		}

		return map;
	}

	void addNamesToSection(IniContents& ini, const wstring& sectionName, const wstring_map& map) {
		ini.removeSection(sectionName);

		for (const auto& name : map) {
			if (name.first.empty()) continue;
			ini.setValue(sectionName, name.first, name.second);
		}
	}
};


class MemoryCacheNameRetriever : public CacheNameRetriever {
public:
	MemoryCacheNameRetriever(NameRetriever& mainRetriever, const function<bool()> reloadCacheGetter) 
		: CacheNameRetriever(mainRetriever, reloadCacheGetter) { }
private:
	unordered_map<string, wstring_map_pair> _mapCache{};
protected:
	wstring_map_pair getMapFromCache(const string& vnId) const override {
		return _mapCache.find(vnId) != _mapCache.end() ? _mapCache.at(vnId) : wstring_map_pair();
	}

	void saveMapToCache(const string& vnId, const wstring_map_pair& map) override {
		if (map.first.empty()) return;
		_mapCache[vnId] = map;
	}
};
