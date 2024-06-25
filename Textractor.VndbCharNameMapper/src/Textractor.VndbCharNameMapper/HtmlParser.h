
#pragma once
#include "Common.h"
#include "GenderStrMapper.h"
#include <functional>
#include <regex>
#include <unordered_set>
#include <vector>
using namespace std;


class HtmlParser {
public:
	virtual ~HtmlParser() { }
	virtual string extractSpoilCharsPath(const string& html) const = 0;
	virtual CharMappings getNameMappings(const string& html) const = 0;
	virtual CharMappings getNameMappings(const wstring& html) const = 0;
	virtual CharMappings convertToNameMappings(const vector<wstring>& jpNames,
		const vector<wstring>& enNames, const vector<Gender>& genders = {}) const = 0;
	virtual vector<wstring> parseAllJpNames(const wstring& html) const = 0;
	virtual vector<wstring> parseAllEnNames(const wstring& html) const = 0;
};


class VndbHtmlParser : public HtmlParser {
public:
	string extractSpoilCharsPath(const string& html) const override;
	CharMappings getNameMappings(const string& html) const override;
	CharMappings getNameMappings(const wstring& html) const override;
	CharMappings convertToNameMappings(const vector<wstring>& jpNames, 
		const vector<wstring>& enNames, const vector<Gender>& genders = {}) const override;
	vector<wstring> parseAllJpNames(const wstring& html) const override;
	vector<wstring> parseAllEnNames(const wstring& html) const override;
private:
	static const unordered_set<wchar_t> _nameDelims;
	static const regex _spoilCharsPathPattern;
	static const wregex _charHeaderPattern;
	static const wregex _jpNamePattern;
	static const wregex _enNamePattern;
	static const wregex _genderPattern;
	const GenderStrMapper& _genderStrMap = VndbHtmlGenderStrMapper();

	vector<wstring> parseAllCharHeaders(const wstring& html) const;
	vector<wstring> parseAllJpNames(const vector<wstring>& htmlSections) const;
	vector<wstring> parseAllEnNames(const vector<wstring>& htmlSections) const;
	vector<Gender> parseAllGenders(const vector<wstring>& htmlSections) const;

	template<typename T>
	void addIfNotExist(templ_map<T>& map, const wstring& key, const T& value) const;
	wstring removeDelimsFromStr(const wstring& input) const;
	vector<wstring> splitStrByDelims(const wstring& input) const;
	bool isNameDelim(wchar_t ch) const;
	vector<wstring> parseAll(const wstring& html, const wregex& pattern) const;
	vector<wstring> parseAll(const vector<wstring>& htmlSections, const wregex& pattern) const;
	template<typename T>
	vector<T> parseAll(const vector<wstring>& htmlSections, 
		const wregex& pattern, const function<T(const wstring&)>& customValMap) const;
};