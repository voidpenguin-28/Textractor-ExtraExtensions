#pragma once

#include "Extension.h"
#include "ExtensionConfig.h"
#include "NameMapper.h"
#include "NameRetriever.h"

class NameMappingManager {
public:
	virtual ~NameMappingManager() { }
	virtual wstring applyAllNameMappings(const wstring& sentence, SentenceInfoWrapper& sentInfoWrapper) = 0;
};

class DefaultNameMappingManager : public NameMappingManager {
public:
	DefaultNameMappingManager(const ConfigRetriever& configRetriever,
		const NameMapper& nameMapper, NameRetriever& nameRetriever)
		: _configRetriever(configRetriever), _nameMapper(nameMapper), _nameRetriever(nameRetriever) { }

	wstring applyAllNameMappings(const wstring& sentence, SentenceInfoWrapper& sentInfoWrapper) override {
		ExtensionConfig config = _configRetriever.getConfig();
		if (!isAllowed(config, sentInfoWrapper)) return sentence;

		vector<string> vnIdList = getVnIds(config);
		wstring newSentence = applyAllNameMappings(vnIdList, sentence, config.minNameCharSize);
		return newSentence;
	}
private:
	const ConfigRetriever& _configRetriever;
	const NameMapper& _nameMapper;
	NameRetriever& _nameRetriever;

	bool isAllowed(const ExtensionConfig& config, SentenceInfoWrapper& sentInfoWrapper) {
		if (config.disabled) return false;
		if (config.activeThreadOnly && !sentInfoWrapper.isActiveThread()) return false;
		if (!meetsConsoleAndClipboardRequirements(sentInfoWrapper, config)) return false;

		return true;
	}

	bool meetsConsoleAndClipboardRequirements(
		SentenceInfoWrapper& sentInfoWrapper, const ExtensionConfig& config) const
	{
		wstring threadName = sentInfoWrapper.getThreadName();

		switch (config.skipConsoleAndClipboard) {
		case 1:
			if (sentInfoWrapper.threadIsConsoleOrClipboard()) return false;
			break;
		case 2:
			if (threadName == L"Console") return false;
			break;
		case 3:
			if (threadName == L"Clipboard") return false;
			break;
		}

		return true;
	}

	vector<string> getVnIds(const ExtensionConfig& config) const {
		vector<string> vnIdList;
		string vnIds = config.vnIds, delim = config.vnIdDelim;
		size_t start = 0, end = vnIds.find(delim);

		while (end != string::npos) {
			vnIdList.push_back(vnIds.substr(start, end - start));
			start = end + delim.length();
			end = vnIds.find(delim, start);
		}

		vnIdList.push_back(vnIds.substr(start));
		return vnIdList;
	}

	wstring applyAllNameMappings(const vector<string> vnIdList, wstring sentence, int minNameCharSize) const {
		for (const auto& vnId : vnIdList) {
			sentence = applyNameMappings(vnId, sentence, minNameCharSize);
		}

		return sentence;
	}

	wstring applyNameMappings(const string& vnId, const wstring sentence, int minNameCharSize) const {
		wstring_map_pair map = getNameMappings(vnId, minNameCharSize);
		return _nameMapper.applyNameMappings(map, sentence);
	}

	wstring_map_pair getNameMappings(const string& vnId, int minNameCharSize) const {
		wstring_map_pair map = _nameRetriever.getNameMappings(vnId);
		map = filterMap(map, minNameCharSize);
		return map;
	}

	wstring_map_pair filterMap(const wstring_map_pair& map, int minNameCharSize) const {
		wstring_map_pair filteredMap(
			filterMap(map.first, minNameCharSize),
			filterMap(map.second, minNameCharSize)
		);

		return filteredMap;
	}

	wstring_map filterMap(const wstring_map& map, int minNameCharSize) const {
		wstring_map filteredMap{};

		for (const auto& name : map) {
			if (name.first.length() < minNameCharSize) continue;
			filteredMap[name.first] = name.second;
		}

		return filteredMap;
	}
};
