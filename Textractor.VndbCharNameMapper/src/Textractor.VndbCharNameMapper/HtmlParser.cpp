
#include "HtmlParser.h"
#include "Libraries/strhelper.h"
#include <algorithm>
#include <functional>
#include <sstream>

const unordered_set<wchar_t> VndbHtmlParser::_nameDelims = { L' ', L'・' };


const regex VndbHtmlParser::_spoilCharsPathPattern = 
	regex("<a .{0,}href=\"([^\"]+)\"[^>]{0,}>Spoil me!");
const wregex VndbHtmlParser::_charHeaderPattern =
	wregex(L"<div class=\"[^\"]{0,}chardetails[^\"]{0,}\">.+?<thead>(.+?)</thead>");
const wregex VndbHtmlParser::_jpNamePattern = 
	wregex(L"<small [^>]+>([^<]+)");
const wregex VndbHtmlParser::_enNamePattern = 
	wregex(L"<a href=[^>]+>([^<]+)");
const wregex VndbHtmlParser::_genderPattern =
	wregex(L"<abbr class=\"icon-gen[^\"]+\" title=\"([^\"]+)\"");


// *** PUBLIC

string VndbHtmlParser::extractSpoilCharsPath(const string& html) const {
	smatch match;
	
	if (!regex_search(html, match, _spoilCharsPathPattern)) return "";
	return match[1].str();
}

CharMappings VndbHtmlParser::getNameMappings(const string& html) const {
	wstring whtml = StrHelper::convertToW(html);
	return getNameMappings(whtml);
}

CharMappings VndbHtmlParser::getNameMappings(const wstring& html) const {
	vector<wstring> charHeaders = parseAllCharHeaders(html);

	vector<wstring> jpNames = parseAllJpNames(charHeaders);
	vector<wstring> enNames = parseAllEnNames(charHeaders);
	vector<Gender> genders = parseAllGenders(charHeaders);

	CharMappings nameMappings = convertToNameMappings(jpNames, enNames, genders);
	return nameMappings;
}

CharMappings VndbHtmlParser::convertToNameMappings(const vector<wstring>& jpNames, 
	const vector<wstring>& enNames, const vector<Gender>& genders) const
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

vector<wstring> VndbHtmlParser::parseAllJpNames(const wstring& html) const {
	vector<wstring> charHeaders = parseAllCharHeaders(html);
	return parseAllJpNames(charHeaders);
}

vector<wstring> VndbHtmlParser::parseAllEnNames(const wstring& html) const {
	vector<wstring> charHeaders = parseAllCharHeaders(html);
	return parseAllEnNames(charHeaders);
}

// *** PRIVATE

vector<wstring> VndbHtmlParser::parseAllCharHeaders(const wstring& html) const {
	return parseAll(html, _charHeaderPattern);
}

vector<wstring> VndbHtmlParser::parseAllJpNames(const vector<wstring>& htmlSections) const {
	return parseAll(htmlSections, _jpNamePattern);
}

vector<wstring> VndbHtmlParser::parseAllEnNames(const vector<wstring>& htmlSections) const {
	return parseAll(htmlSections, _enNamePattern);
}

vector<Gender> VndbHtmlParser::parseAllGenders(const vector<wstring>& htmlSections) const {
	static const function<Gender(const wstring&)> genderMap = [this](const wstring& gender) {
		return _genderStrMap.map(gender);
	};

	return parseAll(htmlSections, _genderPattern, genderMap);
}

template<typename T>
void VndbHtmlParser::addIfNotExist(templ_map<T>& map, const wstring& key, const T& value) const {
	if (map.count(key)) return;
	map[key] = value;
}

wstring VndbHtmlParser::removeDelimsFromStr(const wstring& input) const {
	wstring output = L"";

	for (auto& ch : input) {
		if (!isNameDelim(ch)) output += ch;
	}

	return output;
}

vector<wstring> VndbHtmlParser::splitStrByDelims(const wstring& input) const {
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

bool VndbHtmlParser::isNameDelim(wchar_t ch) const {
	return _nameDelims.find(ch) != _nameDelims.end();
}

vector<wstring> VndbHtmlParser::parseAll(const wstring& html, const wregex& pattern) const {
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

vector<wstring> VndbHtmlParser::parseAll(const vector<wstring>& htmlSections, const wregex& pattern) const {
	static const function<wstring(const wstring&)> customMap = [](const wstring& v) { return v; };
	return parseAll(htmlSections, pattern, customMap);
}

template<typename T>
vector<T> VndbHtmlParser::parseAll(const vector<wstring>& htmlSections,
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
