#pragma once

#include "HtmlParser.h"
#include "ExtensionConfig.h"
#include <string>
using namespace std;

class NameMapper {
public:
	virtual ~NameMapper() { }
	virtual wstring applyNameMappings(const wstring_map_pair& nameMap, wstring str) const = 0;
};


class DefaultNameMapper : public NameMapper {
public:
	wstring applyNameMappings(const wstring_map_pair& nameMap, wstring str) const override {
		str = replaceNames(nameMap.first, str);
		str = replaceNames(nameMap.second, str);
		return str;
	}
private:
	wstring replaceNames(const wstring_map& nameMap, wstring str) const {
		for (const auto& name : nameMap) {
			if (str.find(name.first) == string::npos) continue;
			str = replace(str, name.first, name.second);
		}

		return str;
	}

	wstring replace(const wstring& input, const wstring& target, const wstring& replacement) const {
		wstring result = input;
		size_t startPos = 0;

		while ((startPos = result.find(target, startPos)) != wstring::npos) {
			result.replace(startPos, target.length(), replacement);
			startPos += replacement.length();
		}

		return result;
	}
};
