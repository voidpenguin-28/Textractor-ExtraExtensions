#pragma once

#include "Extension.h"
#include "ExtensionConfig.h"
#include "ExtExecRequirements.h"
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
	DefaultNameMappingManager(ConfigRetriever& configRetriever,
		const NameMapper& nameMapper, NameRetriever& nameRetriever, 
		const VnIdsParser& vnIdsParser, const ExtExecRequirements& execRequirements)
		: _configRetriever(configRetriever), _nameMapper(nameMapper), 
			_nameRetriever(nameRetriever), _vnIdsParser(vnIdsParser),
			_execRequirements(execRequirements) { }

	wstring applyAllNameMappings(const wstring& sentence, SentenceInfoWrapper& sentInfoWrapper) override {
		ExtensionConfig config = _configRetriever.getConfig(false);
		if (!_execRequirements.meetsRequirements(sentInfoWrapper, config)) return sentence;

		vector<string> vnIdList = _vnIdsParser.parse(config, sentInfoWrapper);
		wstring newSentence = applyAllNameMappings(vnIdList, sentence, config);
		return newSentence;
	}
private:
	ConfigRetriever& _configRetriever;
	const NameMapper& _nameMapper;
	NameRetriever& _nameRetriever;
	const VnIdsParser& _vnIdsParser;
	const ExtExecRequirements& _execRequirements;

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
