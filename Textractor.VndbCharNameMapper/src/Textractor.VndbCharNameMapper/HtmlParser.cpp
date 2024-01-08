
#include "HtmlParser.h"
#include "Libraries/stringconvert.h"
#include <algorithm>
#include <sstream>

const unordered_set<wchar_t> VndbHtmlParser::_nameDelims = { L' ', L'・' };

const regex VndbHtmlParser::_spoilCharsPathPattern = 
	regex("<a .{0,}href=\"([^\"]+)\"[^>]{0,}>Spoil me!");
const wregex VndbHtmlParser::_jpNamePattern = 
	wregex(L"<div class=\"[^\"]{0,}chardetails[^\"]{0,}\">.+?<thead>.+?<small [^>]+>([^<]+)");
const wregex VndbHtmlParser::_enNamePattern = 
	wregex(L"<div class=\"[^\"]{0,}chardetails[^\"]{0,}\">.+?<thead>.+?<a href=[^>]+>([^<]+)");


// *** PUBLIC

string VndbHtmlParser::extractSpoilCharsPath(const string& html) const {
	smatch match;
	
	if (!regex_search(html, match, _spoilCharsPathPattern)) return "";
	return match[1].str();
}

wstring_map_pair VndbHtmlParser::getNameMappings(const string& html) const {
	wstring whtml = convertToW(html);
	return getNameMappings(whtml);
}

wstring_map_pair VndbHtmlParser::getNameMappings(const wstring& html) const {
	vector<wstring> jpNames = parseAllJpNames(html);
	vector<wstring> enNames = parseAllEnNames(html);
	wstring_map_pair nameMappings = convertToNameMappings(jpNames, enNames);
	return nameMappings;
}

wstring_map_pair VndbHtmlParser::convertToNameMappings(
	const vector<wstring>& jpNames, const vector<wstring>& enNames) const
{
	wstring_map fullNameMap = { };
	wstring_map singleNameMap = { };

	if (jpNames.size() != enNames.size())
		throw runtime_error("Number of parsed JP & EN names do not match; JP: " + to_string(jpNames.size()) + ", EN: " + to_string(enNames.size()));

	for (size_t i = 0; i < jpNames.size(); i++) {
		if (jpNames[i].empty()) continue;

		addIfNotExist(fullNameMap, jpNames[i], enNames[i]);
		addIfNotExist(fullNameMap, removeDelimsFromStr(jpNames[i]), enNames[i]);

		vector<wstring> jpNameParts = splitStrByDelims(jpNames[i]);
		vector<wstring> enNameParts = splitStrByDelims(enNames[i]);
		if (jpNameParts.size() != enNameParts.size() || jpNameParts.size() <= 1) continue;

		for (size_t j = 0; j < jpNameParts.size(); j++) {
			if (jpNameParts[j].empty()) continue;
			addIfNotExist(singleNameMap, jpNameParts[j], enNameParts[j]);
		}
	}

	return wstring_map_pair(fullNameMap, singleNameMap);
}

vector<wstring> VndbHtmlParser::parseAllJpNames(const wstring& html) const {
	return parseAllNames(html, _jpNamePattern);
}

vector<wstring> VndbHtmlParser::parseAllEnNames(const wstring& html) const {
	return parseAllNames(html, _enNamePattern);
}


// *** PRIVATE

void VndbHtmlParser::addIfNotExist(wstring_map& map, const wstring& key, const wstring& value) const {
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

vector<wstring> VndbHtmlParser::parseAllNames(const wstring& html, const wregex& pattern) const {
	wsregex_iterator iterator(html.begin(), html.end(), pattern);
	wsregex_iterator end;

	vector<wstring> names;

	while (iterator != end) {
		wsmatch match = *iterator;
		names.push_back(match[1].str());
		++iterator;
	}

	return names;
}
