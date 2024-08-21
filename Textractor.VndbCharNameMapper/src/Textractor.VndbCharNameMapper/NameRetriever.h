#pragma once

#include "Libraries/inihandler.h"
#include "Libraries/Locker.h"
#include "Libraries/strhelper.h"
#include "HtmlParsers/HtmlParser.h"
#include "GenderStrMapper.h"
#include "HttpClient.h"
#include "ExtensionConfig.h"
#include <functional>
#include <string>
using namespace std;


class NameRetriever {
public:
	virtual ~NameRetriever() { }
	virtual CharMappings getNameMappings(const string& vnId) = 0;
};


class NoNameRetriever : public NameRetriever {
public:
	CharMappings getNameMappings(const string& vnId) override {
		return CharMappings();
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

	CharMappings getNameMappings(const string& vnId) override {
		if (vnId.empty()) return CharMappings();
		string url = createUrl(vnId);
		string html = getAllCharsHtmlPage(url);

		CharMappings nameMap = _htmlParser.getNameMappings(html);
		return nameMap;
	}
private:
	const vector<string> _httpHeaders = { "Cookie: vndb_samesite=1" };
	const function<string()> _urlTemplateGetter;
	const HttpClient& _httpClient;
	const HtmlParser& _htmlParser;

	string createUrl(string vnId) const {
		static const string errMsg = "UrlTemplate config value is invalid. Must contain '{0}' value to indicate expected location of vn_id (ex: https://vndb.org/{0}/chars). Value: ";
		static const string tmplPar = "{0}";
		if (vnId[0] != 'v') vnId = 'v' + vnId;

		string urlTemplate = _urlTemplateGetter();
		size_t tmplIndex = urlTemplate.find(tmplPar);
		if (tmplIndex == string::npos) throw runtime_error(errMsg + urlTemplate);

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

	CharMappings getNameMappings(const string& vnId) override {
		if (vnId.empty()) return CharMappings();
		bool reloadCache = _reloadCacheGetter();
		CharMappings map;
		
		_locker.lock([this, &vnId, reloadCache, &map]() {
			if (!reloadCache) {
				map = getMapFromCache(vnId);
				if (!map.fullNameMap.empty()) return;
			}

			map = _mainRetriever.getNameMappings(vnId);
			saveMapToCache(vnId, map);
		});

		return map;
	}
protected:
	NameRetriever& _mainRetriever;
	const function<bool()> _reloadCacheGetter;
	mutable BasicLocker _locker;

	virtual CharMappings getMapFromCache(const string& vnId) const = 0;
	virtual void saveMapToCache(const string& vnId, const CharMappings& map) = 0;
};


class IniFileCacheNameRetriever : public CacheNameRetriever {
public:
	IniFileCacheNameRetriever(const string& iniFileName, NameRetriever& mainNameRetriever, 
		const GenderStrMapper& genderStrMap, const function<bool()> reloadCacheGetter) 
		: CacheNameRetriever(mainNameRetriever, reloadCacheGetter), 
			_iniFileName(iniFileName), _iniHandler(iniFileName), _genderStrMap(genderStrMap) { }
protected:
	const GenderStrMapper& _genderStrMap;
	const string _iniFileName;
	const IniFileHandler _iniHandler;

	CharMappings getMapFromCache(const string& vnId) const override {
		auto ini = unique_ptr<IniContents>(_iniHandler.readIni());

		wstring fullNameSection = createFullNameSectionName(vnId);
		if (!ini->sectionExists(fullNameSection)) return CharMappings();
		wstring_map fullNameMap = getNamesFromSection(*ini, fullNameSection);

		wstring singleNameSection = createSingleNameSectionName(vnId);
		wstring_map singleNameMap = getNamesFromSection(*ini, singleNameSection);

		wstring genderSection = createGenderSectionName(vnId);
		if (!ini->sectionExists(genderSection)) return CharMappings();
		gender_map genderMap = getGendersFromSection(*ini, genderSection);

		return CharMappings(fullNameMap, singleNameMap, genderMap);
	}

	void saveMapToCache(const string& vnId, const CharMappings& map) override {
		auto ini = unique_ptr<IniContents>(_iniHandler.readIni());
		
		wstring fullNameSection = createFullNameSectionName(vnId);
		addNamesToSection(*ini, fullNameSection, map.fullNameMap);

		wstring singleNameSection = createSingleNameSectionName(vnId);
		addNamesToSection(*ini, singleNameSection, map.singleNameMap);

		wstring genderSection = createGenderSectionName(vnId);
		addGendersToSection(*ini, genderSection, map.genderMap);

		_iniHandler.saveIni(*ini);
	}
private:
	wstring createFullNameSectionName(const string& vnId) const {
		return StrHelper::convertToW(vnId) + L"-Full";
	}

	wstring createSingleNameSectionName(const string& vnId) const {
		return StrHelper::convertToW(vnId) + L"-Single";
	}

	wstring createGenderSectionName(const string& vnId) const {
		return StrHelper::convertToW(vnId) + L"-Gender";
	}

	wstring_map getNamesFromSection(const IniContents& ini, const wstring& sectionName) const {
		vector<pair<wstring, wstring>> fullNamesList = ini.getAllValues(sectionName);
		wstring_map nameMap = pairVectorToMap(fullNamesList);
		return nameMap;
	}

	gender_map getGendersFromSection(const IniContents& ini, const wstring& sectionName) const {
		wstring_map genderStrMap = getNamesFromSection(ini, sectionName);
		return convertTo(genderStrMap);
	}

	wstring_map pairVectorToMap(vector<pair<wstring, wstring>> pairVector) const {
		wstring_map map{};

		for (const auto& p : pairVector) {
			if (p.first.empty()) continue;
			map[p.first] = p.second;
		}

		return map;
	}

	gender_map convertTo(const wstring_map& map) const {
		gender_map genders{};

		for (const auto& m : map) {
			genders[m.first] = _genderStrMap.map(m.second);
		}

		return genders;
	}

	wstring_map convertTo(const gender_map& map) const {
		wstring_map genderStrs{};

		for (const auto& m : map) {
			genderStrs[m.first] = _genderStrMap.map(m.second);
		}

		return genderStrs;
	}

	void addNamesToSection(IniContents& ini, const wstring& sectionName, const wstring_map& map) {
		ini.removeSection(sectionName);

		for (const auto& name : map) {
			if (name.first.empty()) continue;
			ini.setValue(sectionName, name.first, name.second);
		}
	}

	void addGendersToSection(IniContents& ini, const wstring& sectionName, const gender_map& map) {
		wstring_map strMap = convertTo(map);
		addNamesToSection(ini, sectionName, strMap);
	}
};


class MemoryCacheNameRetriever : public CacheNameRetriever {
public:
	MemoryCacheNameRetriever(NameRetriever& mainRetriever, const function<bool()> reloadCacheGetter) 
		: CacheNameRetriever(mainRetriever, reloadCacheGetter) { }
private:
	unordered_map<string, CharMappings> _mapCache{};
protected:
	CharMappings getMapFromCache(const string& vnId) const override {
		return _mapCache.find(vnId) != _mapCache.end() ? _mapCache.at(vnId) : CharMappings();
	}

	void saveMapToCache(const string& vnId, const CharMappings& map) override {
		if (map.fullNameMap.empty()) return;
		_mapCache[vnId] = map;
	}
};
