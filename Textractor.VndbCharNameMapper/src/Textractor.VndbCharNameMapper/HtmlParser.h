
#pragma once
#include "Common.h"
#include <regex>
#include <unordered_set>
#include <vector>
using namespace std;


class HtmlParser {
public:
	virtual ~HtmlParser() { }
	virtual string extractSpoilCharsPath(const string& html) const = 0;
	virtual wstring_map_pair getNameMappings(const string& html) const = 0;
	virtual wstring_map_pair getNameMappings(const wstring& html) const = 0;
	virtual wstring_map_pair convertToNameMappings(
		const vector<wstring>& jpNames, const vector<wstring>& enNames) const = 0;
	virtual vector<wstring> parseAllJpNames(const wstring& html) const = 0;
	virtual vector<wstring> parseAllEnNames(const wstring& html) const = 0;
};


class VndbHtmlParser : public HtmlParser {
public:
	string extractSpoilCharsPath(const string& html) const override;
	wstring_map_pair getNameMappings(const string& html) const override;
	wstring_map_pair getNameMappings(const wstring& html) const override;
	wstring_map_pair convertToNameMappings(
		const vector<wstring>& jpNames, const vector<wstring>& enNames) const override;
	vector<wstring> parseAllJpNames(const wstring& html) const override;
	vector<wstring> parseAllEnNames(const wstring& html) const override;
private:
	static const unordered_set<wchar_t> _nameDelims;
	static const regex _spoilCharsPathPattern;
	static const wregex _jpNamePattern;
	static const wregex _enNamePattern;

	void addIfNotExist(wstring_map& map, const wstring& key, const wstring& value) const;
	wstring removeDelimsFromStr(const wstring& input) const;
	vector<wstring> splitStrByDelims(const wstring& input) const;
	bool isNameDelim(wchar_t ch) const;
	vector<wstring> parseAllNames(const wstring& html, const wregex& pattern) const;
};