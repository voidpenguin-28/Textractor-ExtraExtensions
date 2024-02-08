#pragma once

#include <codecvt>
#include <string>
#include <vector>
using namespace std;

class StrHelper {
public:
	static wstring replace(wstring str, const wstring& target, const wstring& replacement) {
		if (str.empty()) return str;
		size_t targetIndex;

		while ((targetIndex = str.find(target)) != wstring::npos) {
			str = str.erase(targetIndex, target.length());
			str = str.insert(targetIndex, replacement);
		}

		return str;
	}

	static wstring join(const wstring& delim, const vector<wstring>& strArr) {
		wstring str = L"";

		for (size_t i = 0; i < strArr.size(); i++) {
			str += strArr[i] + delim;
		}

		if (!str.empty()) str = rtrim(str, delim);
		return str;
	}

	static wstring rtrim(wstring s, const wstring& trim) {
		size_t trLen = trim.length();

		while (s.length() >= trLen && s.substr(s.length() - trLen) == trim)
			s = s.erase(s.length() - trLen);

		return s;
	}

	static wstring rtruncate(const wstring& str, size_t truncLen) {
		return str.length() > truncLen ? str.substr(str.length() - truncLen) : str;
	}

	static wstring convertToW(const string& str) {
		return wstring_convert<codecvt_utf8_utf16<wchar_t>>().from_bytes(str);
	}

	static string convertFromW(const wstring& str) {
		return wstring_convert<codecvt_utf8_utf16<wchar_t>>().to_bytes(str);
	}
};
