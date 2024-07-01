
#pragma once
#include "HtmlParser.h"
#include "../Libraries/strhelper.h"
#include "../CharMappingConverter.h"
#include "../GenderStrMapper.h"
#include <algorithm>
#include <functional>
#include <regex>
#include <sstream>


class RegexVndbHtmlParser : public HtmlParser {
public:
	RegexVndbHtmlParser(const CharMappingConverter& charMappingConverter, const GenderStrMapper& genderStrMap)
		: _charMappingConverter(charMappingConverter), _genderStrMap(genderStrMap) { }

	string extractSpoilCharsPath(const string& html) const {
		smatch match;

		if (!regex_search(html, match, _spoilCharsPathPattern)) return "";
		return match[1].str();
	}

	CharMappings getNameMappings(const string& html) const {
		wstring whtml = StrHelper::convertToW(html);
		return getNameMappings(whtml);
	}

	CharMappings getNameMappings(const wstring& html) const {
		vector<wstring> charHeaders = parseAllCharHeaders(html);

		vector<wstring> jpNames = parseAllJpNames(charHeaders);
		vector<wstring> enNames = parseAllEnNames(charHeaders);
		vector<Gender> genders = parseAllGenders(charHeaders);

		CharMappings nameMappings = _charMappingConverter.convert(jpNames, enNames, genders);
		return nameMappings;
	}

	vector<wstring> parseAllJpNames(const wstring& html) const {
		vector<wstring> charHeaders = parseAllCharHeaders(html);
		return parseAllJpNames(charHeaders);
	}

	vector<wstring> parseAllEnNames(const wstring& html) const {
		vector<wstring> charHeaders = parseAllCharHeaders(html);
		return parseAllEnNames(charHeaders);
	}
private:
	const regex _spoilCharsPathPattern =
		regex("<a .{0,}href=\"([^\"]+)\"[^>]{0,}>Spoil me!");
	const wregex _charHeaderPattern =
		wregex(L"<div class=\"[^\"]{0,}chardetails[^\"]{0,}\">.+?<thead>(.+?)</thead>");
	const wregex _jpNamePattern =
		wregex(L"<small [^>]+>([^<]+)");
	const wregex _enNamePattern =
		wregex(L"<a href=[^>]+>([^<]+)");
	const wregex _genderPattern =
		wregex(L"<abbr class=\"icon-gen[^\"]+\" title=\"([^\"]+)\"");

	const CharMappingConverter& _charMappingConverter;
	const GenderStrMapper& _genderStrMap;


	vector<wstring> parseAllCharHeaders(const wstring& html) const {
		return parseAll(html, _charHeaderPattern);
	}

	vector<wstring> parseAllJpNames(const vector<wstring>& htmlSections) const {
		return parseAll(htmlSections, _jpNamePattern);
	}

	vector<wstring> parseAllEnNames(const vector<wstring>& htmlSections) const {
		return parseAll(htmlSections, _enNamePattern);
	}

	vector<Gender> parseAllGenders(const vector<wstring>& htmlSections) const {
		static const function<Gender(const wstring&)> genderMap = [this](const wstring& gender) {
			return _genderStrMap.map(gender);
		};

		return parseAll(htmlSections, _genderPattern, genderMap);
	}

	vector<wstring> parseAll(const wstring& html, const wregex& pattern) const {
		wsregex_iterator iterator(html.begin(), html.end(), pattern);
		wsregex_iterator end;
		vector<wstring> values;

		while (iterator != end) {
			wsmatch match = *iterator;
			values.push_back(match[1].str());
			++iterator;
		}

		return values;
	}

	vector<wstring> parseAll(const vector<wstring>& htmlSections, const wregex& pattern) const {
		static const function<wstring(const wstring&)> customMap = [](const wstring& v) { return v; };
		return parseAll(htmlSections, pattern, customMap);
	}

	template<typename T>
	vector<T> parseAll(const vector<wstring>& htmlSections,
		const wregex& pattern, const function<T(const wstring&)>& customValMap) const
	{
		vector<T> mappings{};
		wsmatch match;
		wstring matchVal;
		T mapVal;

		for (const wstring& section : htmlSections) {
			matchVal = regex_search(section, match, pattern) ? match[1].str() : L"";
			mapVal = customValMap(matchVal);
			mappings.push_back(mapVal);
		}

		return mappings;
	}
};
