
#pragma once
#include "../_Libraries/strhelper.h"
#include <unordered_set>


class GptLineParser {
public:
	virtual ~GptLineParser() { }
	virtual wstring parseLastLine(const wstring& response) const = 0;
};


class DefaultGptLineParser : public GptLineParser {
public:
	wstring parseLastLine(const wstring& response) const override {
		size_t startIndex = response.length(), newStartIndex;

		do {
			startIndex = response.rfind(L'\n', startIndex - 1);

			if (startIndex == wstring::npos) {
				startIndex = 0;
				if (!isTransLineStart(response, startIndex, newStartIndex))
					newStartIndex = startIndex;

				break;
			}
		} while (!isTransLineStart(response, startIndex + 1, newStartIndex));

		return response.substr(newStartIndex);
	}
private:
	bool isTransLineStart(const wstring& str, size_t startIndex, size_t& newStartIndex) const {
		static const unordered_set<wchar_t> _seps{ L':', L'.' };
		newStartIndex = wstring::npos;
		int i = 0;
		while (startIndex + i < str.length() && isdigit(str[startIndex + i])) i++;

		if (i == 0 || startIndex + i >= str.length() - 1) return false;
		if (_seps.find(str[startIndex + i]) == _seps.end()) return false;
		if (str[startIndex + ++i] != L' ') return false;

		newStartIndex = startIndex + i + 1;
		return true;
	}
};
