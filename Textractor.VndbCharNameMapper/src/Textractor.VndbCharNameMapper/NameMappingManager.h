#pragma once

#include "Extension.h"
#include "ExtensionConfig.h"
#include "NameMapper.h"
#include "NameRetriever.h"
#include "VnIdsParser.h"


class NameMappingManager {
public:
	virtual ~NameMappingManager() { }
	virtual wstring applyAllNameMappings(const wstring& sentence, SentenceInfoWrapper& sentInfoWrapper) = 0;
};


class DefaultNameMappingManager : public NameMappingManager {
public:
	DefaultNameMappingManager(const ConfigRetriever& configRetriever,
		const NameMapper& nameMapper, NameRetriever& nameRetriever, const VnIdsParser& vnIdsParser)
		: _configRetriever(configRetriever), _nameMapper(nameMapper), 
			_nameRetriever(nameRetriever), _vnIdsParser(vnIdsParser) { }

	wstring applyAllNameMappings(const wstring& sentence, SentenceInfoWrapper& sentInfoWrapper) override {
		ExtensionConfig config = _configRetriever.getConfig();
		if (!isAllowed(config, sentInfoWrapper)) return sentence;

		vector<string> vnIdList = _vnIdsParser.parse(config, sentInfoWrapper);
		wstring newSentence = applyAllNameMappings(vnIdList, sentence, config);
		return newSentence;
	}
private:
	const ConfigRetriever& _configRetriever;
	const NameMapper& _nameMapper;
	NameRetriever& _nameRetriever;
	const VnIdsParser& _vnIdsParser;

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

	wstring applyAllNameMappings(const vector<string> vnIdList, 
		wstring sentence, const ExtensionConfig& config) const
	{
		for (const auto& vnId : vnIdList) {
			sentence = applyNameMappings(vnId, sentence, config);
		}

		return sentence;
	}

	wstring applyNameMappings(const string& vnId, const wstring sentence, const ExtensionConfig& config) const {
		CharMappings map = getNameMappings(vnId, config.minNameCharSize);
		return _nameMapper.applyNameMappings(config.mappingMode, map, sentence);
	}

	CharMappings getNameMappings(const string& vnId, int minNameCharSize) const {
		CharMappings map = _nameRetriever.getNameMappings(vnId);
		map = filterMap(map, minNameCharSize);
		return map;
	}

	CharMappings filterMap(const CharMappings& map, int minNameCharSize) const {
		CharMappings filteredMap(
			filterMap(map.fullNameMap, minNameCharSize),
			filterMap(map.singleNameMap, minNameCharSize),
			map.genderMap
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
