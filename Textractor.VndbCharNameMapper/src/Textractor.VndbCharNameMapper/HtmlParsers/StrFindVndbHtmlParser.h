
#pragma once
#include "../Libraries/strhelper.h"
#include "HtmlParser.h"
#include "../CharMappingConverter.h"
#include "../GenderStrMapper.h"


class StrFindVndbHtmlParser : public HtmlParser {
public:
	StrFindVndbHtmlParser(const CharMappingConverter& charMappingConverter, const GenderStrMapper& genderStrMap) 
		: _charMappingConverter(charMappingConverter), _genderStrMap(genderStrMap) { }

	string extractSpoilCharsPath(const string& html) const override {
		size_t startIndex = html.find(_spoilCharsPathBwFindSubStrs[0]);
		if (!validIndex(startIndex)) return "";

		for(size_t i = 1; i < _spoilCharsPathBwFindSubStrs.size(); i++) {
			startIndex = html.rfind(_spoilCharsPathBwFindSubStrs[i], startIndex);
			if (!validIndex(startIndex)) return "";
		}

		startIndex += _spoilCharsPathBwFindSubStrs.back().length();
		size_t endIndex = html.find(_spoilCharsPathEnd, startIndex);
		return validIndex(endIndex) ? html.substr(startIndex, endIndex - startIndex) : "";
	}

	CharMappings getNameMappings(const string& html) const override {
		return getNameMappings(StrHelper::convertToW(html));
	}

	CharMappings getNameMappings(const wstring& html) const override {
		static const vector<pair<vector<wstring>, wstring>> _patterns{
			pair<vector<wstring>, wstring>(_jpNameFindSubStrs, _jpNameEnd),
			pair<vector<wstring>, wstring>(_enNameFindSubStrs, _enNameEnd),
			pair<vector<wstring>, wstring>(_genderFindSubStrs, _genderEnd),
		};

		vector<vector<wstring>> matches = parseAll(html, _patterns);
		vector<Gender> genders = convert(matches[2]);
		return _charMappingConverter.convert(matches[0], matches[1], genders);
	}

	vector<wstring> parseAllJpNames(const wstring& html) const override {
		return parseAll(html, _jpNameFindSubStrs, _jpNameEnd);
	}
	
	vector<wstring> parseAllEnNames(const wstring& html) const override {
		return parseAll(html, _enNameFindSubStrs, _enNameEnd);
	}
private:
	const pair<wstring, size_t> NO_SECTION_MATCH = pair<wstring, size_t>(L"", wstring::npos);
	const CharMappingConverter& _charMappingConverter;
	const GenderStrMapper& _genderStrMap;

	const vector<string> _spoilCharsPathBwFindSubStrs {
		">Spoil me!", "href=\""
	};
	const string _spoilCharsPathEnd = "\"";

	const vector<wstring> _charHeaderFindSubStrs {
		L"<div ", L"chardetails", L"<thead>"
	};
	const wstring _charHeaderEnd = L"</thead>";

	const vector<wstring> _jpNameFindSubStrs {
		L"<small ", L">"
	};
	const wstring _jpNameEnd = L"</";

	const vector<wstring> _enNameFindSubStrs {
		L"<a href=\"", L">"
	};
	const wstring _enNameEnd = L"</";

	const vector<wstring> _genderFindSubStrs {
		L"<abbr", L"icon-gen-", L"title=\""
	};
	const wstring _genderEnd = L"\"";

	bool validIndex(const size_t i) const {
		return i != wstring::npos;
	}

	vector<wstring> parseAll(const wstring& html,
		const vector<wstring>& findSubStrs, const wstring& sectionEnd) const
	{
		vector<vector<wstring>> matches = parseAll(html,
			{ pair<vector<wstring>, wstring>(findSubStrs, sectionEnd) });

		return matches[0];
	}

	vector<vector<wstring>> parseAll(const wstring& html, 
		const vector<pair<vector<wstring>, wstring>>& findPatterns) const
	{
		vector<vector<wstring>> matches(findPatterns.size(), vector<wstring>{});
		pair<wstring, size_t> charHeaderMatch, match;
		size_t startIndex = 0;

		while (startIndex < html.length()) {
			charHeaderMatch = parseSection(html, _charHeaderFindSubStrs, _charHeaderEnd, startIndex);
			startIndex = charHeaderMatch.second;
			if (!validIndex(startIndex)) break;

			for(size_t i = 0; i < findPatterns.size(); i++) {
				match = parseSection(charHeaderMatch.first, findPatterns[i].first, findPatterns[i].second);
				matches[i].push_back(validIndex(match.second) ? match.first : L"");
			}
		}

		return matches;
	}

	pair<wstring, size_t> parseSection(const wstring& section,
		const vector<wstring>& findSubStrs, const wstring& sectionEnd, size_t startIndex = 0) const
	{
		for (const wstring& findSubstr : findSubStrs) {
			startIndex = section.find(findSubstr, startIndex);
			if (!validIndex(startIndex)) return NO_SECTION_MATCH;
			startIndex += findSubstr.length();
		}

		size_t endIndex = section.find(sectionEnd, startIndex);
		if (!validIndex(endIndex)) return NO_SECTION_MATCH;

		wstring match = section.substr(startIndex, endIndex - startIndex);
		return pair<wstring, size_t>(match, endIndex + sectionEnd.length());
	}

	vector<Gender> convert(const vector<wstring> genderStrs) const {
		vector<Gender> genders{};

		for (const wstring& genderStr : genderStrs) {
			genders.push_back(_genderStrMap.map(genderStr));
		}

		return genders;
	}
};
