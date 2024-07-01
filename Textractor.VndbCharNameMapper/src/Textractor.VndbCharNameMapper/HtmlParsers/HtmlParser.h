
#pragma once
#include "../Common.h"

class HtmlParser {
public:
	virtual ~HtmlParser() { }
	virtual string extractSpoilCharsPath(const string& html) const = 0;
	virtual CharMappings getNameMappings(const string& html) const = 0;
	virtual CharMappings getNameMappings(const wstring& html) const = 0;
	virtual vector<wstring> parseAllJpNames(const wstring& html) const = 0;
	virtual vector<wstring> parseAllEnNames(const wstring& html) const = 0;
};
