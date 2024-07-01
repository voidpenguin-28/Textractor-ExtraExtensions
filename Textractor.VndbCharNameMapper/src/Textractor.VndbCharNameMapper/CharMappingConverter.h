
#pragma once
#include "Common.h"
#include <stdexcept>
#include <unordered_set>


class CharMappingConverter {
public:
	virtual ~CharMappingConverter() { }
	virtual CharMappings convert(const vector<wstring>& jpNames,
		const vector<wstring>& enNames, const vector<Gender>& genders = {}) const = 0;
};


class DefaultCharMappingConverter : public CharMappingConverter {
public:
	CharMappings convert(const vector<wstring>& jpNames,
		const vector<wstring>& enNames, const vector<Gender>& genders) const override
	{
		wstring_map fullNameMap = { };
		wstring_map singleNameMap = { };
		gender_map genderMap = { };

		if (jpNames.size() != enNames.size())
			throw runtime_error("Number of parsed JP & EN names do not match; JP: " + to_string(jpNames.size()) + ", EN: " + to_string(enNames.size()));

		for (size_t i = 0; i < jpNames.size(); i++) {
			if (jpNames[i].empty()) continue;

			addIfNotExist(fullNameMap, jpNames[i], enNames[i]);
			addIfNotExist(fullNameMap, removeDelimsFromStr(jpNames[i]), enNames[i]);

			vector<wstring> jpNameParts = splitStrByDelims(jpNames[i]);
			vector<wstring> enNameParts = splitStrByDelims(enNames[i]);

			addIfNotExist(genderMap, enNameParts[enNameParts.size() - 1], genders[i]);
			if (jpNameParts.size() != enNameParts.size() || jpNameParts.size() <= 1) continue;

			for (size_t j = 0; j < jpNameParts.size(); j++) {
				if (jpNameParts[j].empty()) continue;
				addIfNotExist(singleNameMap, jpNameParts[j], enNameParts[j]);
			}
		}

		return CharMappings(fullNameMap, singleNameMap, genderMap);
	}
private:
	const unordered_set<wchar_t> _nameDelims = { L' ', L'・' };

	template<typename T>
	void addIfNotExist(templ_map<T>& map, const wstring& key, const T& value) const {
		if (map.count(key)) return;
		map[key] = value;
	}

	wstring removeDelimsFromStr(const wstring& input) const {
		wstring output = L"";

		for (auto& ch : input) {
			if (!isNameDelim(ch)) output += ch;
		}

		return output;
	}

	vector<wstring> splitStrByDelims(const wstring& input) const {
		vector<wstring> splitStrs = { };
		wstring output = L"";

		for (auto& ch : input) {
			if (!isNameDelim(ch)) {
				output += ch;
			}
			else {
				splitStrs.push_back(output);
				output = L"";
			}
		}

		splitStrs.push_back(output);
		return splitStrs;
	}

	bool isNameDelim(wchar_t ch) const {
		return _nameDelims.find(ch) != _nameDelims.end();
	}
};
