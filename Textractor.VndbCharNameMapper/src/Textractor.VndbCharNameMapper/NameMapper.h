#pragma once

#include "Libraries/strhelper.h"
#include "HtmlParser.h"
#include "ExtensionConfig.h"
#include "GenderStrMapper.h"
#include <string>
using namespace std;

class NameMapper {
public:
	virtual ~NameMapper() { }
	virtual wstring applyNameMappings(MappingMode mappingMode, const CharMappings& charMap, wstring str) const = 0;
};


class DefaultNameMapper : public NameMapper {
public:
	wstring applyNameMappings(MappingMode mappingMode, const CharMappings& charMap, wstring str) const override {
		if (mappingMode == MappingMode::None) return str;

		str = replaceNames(mappingMode, charMap.fullNameMap, charMap.genderMap, str);
		str = replaceNames(mappingMode, charMap.singleNameMap, charMap.genderMap, str);
		return str;
	}
private:
	const GenderStrMapper& _genderStrMapper = DefaultGenderStrMapper();

	wstring replaceNames(MappingMode mappingMode, const wstring_map& nameMap, 
		const gender_map& genderMap, wstring str) const
	{
		if (mappingMode == MappingMode::None) return str;
		wstring mappedName;

		for (const auto& name : nameMap) {
			if (str.find(name.first) == string::npos) continue;

			mappedName = name.second;
			if (mappingMode == MappingMode::NameAndGender) 
				mappedName = appendGenderToName(mappedName, genderMap);

			str = StrHelper::replace<wchar_t>(str, name.first, mappedName);
		}

		return str;
	}

	wstring appendGenderToName(wstring name, const gender_map& genderMap) const {
		Gender gender = parseGender(name, genderMap);
		if (gender == Gender::Unknown) return name;

		wstring genderStr = _genderStrMapper.map(gender);
		return name + L" (" + genderStr + L")";
	}

	Gender parseGender(const wstring& name, const gender_map& genderMap) const {
		wstring newName = StrHelper::split<wchar_t>(name, L' ', true).back();
		Gender gender = genderMap.find(newName) != genderMap.end() ? genderMap.at(newName) : Gender::Unknown;
		return gender;
	}
};
