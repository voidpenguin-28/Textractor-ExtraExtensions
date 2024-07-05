
#pragma once
#include "GptMsgCreator.h"
#include "../NameMapping/VnIdsRetriever.h"
#include "../NameMapping/NameRetriever.h"
using NameMappingMode = ExtensionConfig::NameMappingMode;


class DefaultSysGptMsgCreator : public GptMsgCreator {
public:
	wstring createMsg(const ExtensionConfig& config, SentenceInfoWrapper& sentInfoWrapper) override {
		return config.sysMsgPrefix;
	}
};


class NameMappingSysGptMsgCreator : public GptMsgCreator {
public:
	NameMappingSysGptMsgCreator(VnIdsRetriever& vnIdsRetriever, 
		NameRetriever& nameRetriever, GenderStrMapper& genderStrMapper) 
		: _vnIdsRetriever(vnIdsRetriever), _nameRetriever(nameRetriever), _genderStrMapper(genderStrMapper) { }

	wstring createMsg(const ExtensionConfig& config, SentenceInfoWrapper& sentInfoWrapper) override {
		if (config.nameMappingMode == NameMappingMode::None) return L"";
		vector<string> vnIds = _vnIdsRetriever.getVnIds(sentInfoWrapper);
		if (vnIds.empty()) return L"";

		return createMsg(config.nameMappingMode, vnIds);
	}
private:
	const wstring _nameMapModePrefix = L"Use the following name mappings: ";
	const wstring _nameGenderMapModePrefix = L"Use the following name mappings, and gender mappings when provided: ";
	const wstring _nameMapDelim = L"; ";

	VnIdsRetriever& _vnIdsRetriever;
	NameRetriever& _nameRetriever;
	GenderStrMapper& _genderStrMapper;

	wstring createMsg(NameMappingMode mappingMode, const vector<string>& vnIds) {
		wstring sysMsg = getSysMsgPrefix(mappingMode);
		
		for (const string& vnId : vnIds) {
			sysMsg += createMappings(mappingMode, vnId);
		}

		return sysMsg;
	}

	wstring getSysMsgPrefix(NameMappingMode mappingMode) {
		switch (mappingMode) {
		case NameMappingMode::Name:
			return _nameMapModePrefix;
		case NameMappingMode::NameAndGender:
			return _nameGenderMapModePrefix;
		default:
			return L"";
		}
	}

	wstring createMappings(NameMappingMode mappingMode, const string& vnId) {
		CharMappings charMappings = _nameRetriever.getNameMappings(vnId);
		bool nameGenderMode = mappingMode == NameMappingMode::NameAndGender;
		wstring mappings = L"";

		for (const auto& name : charMappings.singleNameMap) {
			mappings += name.first + L'=' + name.second;
			mappings += createGenderAppend(mappingMode, charMappings.genderMap, name.second);
			mappings += _nameMapDelim;
		}

		return mappings;
	}

	wstring createGenderAppend(NameMappingMode mappingMode, const gender_map& genderMap, const wstring& enName) {
		if (mappingMode != NameMappingMode::NameAndGender) return L"";
		if (!contains(genderMap, enName)) return L"";

		Gender gender = genderMap.at(enName);
		if (gender == Gender::Unknown) return L"";

		return formatGenderMsg(gender);
	}

	template<typename T>
	bool contains(unordered_map<wstring, T> map, const wstring& key) {
		return map.find(key) != map.end();
	}

	wstring formatGenderMsg(Gender gender) {
		wstring genderStr = _genderStrMapper.map(gender);
		return L" (" + genderStr + L")";
	}
};
