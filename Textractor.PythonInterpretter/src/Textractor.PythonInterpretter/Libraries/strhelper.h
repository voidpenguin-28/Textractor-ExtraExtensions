#pragma once

#include <string>
#include <vector>
#include <windows.h>
using namespace std;

template<class charT>
using string_base = basic_string<charT, char_traits<charT>, allocator<charT>>;


class StrHelper {
private:
	template<class charT>
	static pair<string_base<charT>, string_base<charT>> getDefaultPlaceholderWrapper() {
		static const string_base<charT> _prfx(1, (charT)123); // "{"
		static const string_base<charT> _sfx(1, (charT)125); // "}"
		static const pair<string_base<charT>, string_base<charT>> _wrapper(_prfx, _sfx);

		return _wrapper;
	}
public:
	template<class charT>
	static string_base<charT> replace(string_base<charT> str,
		const string_base<charT>& target, const string_base<charT>& replacement)
	{
		if (str.empty()) return str;
		size_t targetIndex;

		while ((targetIndex = str.find(target)) != string_base<charT>::npos) {
			str = str.erase(targetIndex, target.length());
			str = str.insert(targetIndex, replacement);
		}

		return str;
	}

	template<class charT>
	static string_base<charT> join(const string_base<charT>& delim, const vector<string_base<charT>>& strArr) {
		string_base<charT> str = getBlank<charT>();

		for (size_t i = 0; i < strArr.size(); i++) {
			str += strArr[i] + delim;
		}

		if (!str.empty()) str = rtrim<charT>(str, delim);
		return str;
	}

	template<class charT>
	static string_base<charT> trim(string_base<charT> s, const string_base<charT>& trim) {
		return rtrim<charT>(ltrim(s, trim), trim);
	}

	template<class charT>
	static string_base<charT> ltrim(string_base<charT> s, const string_base<charT>& trim) {
		size_t trLen = trim.length();

		while (s.length() >= trLen && s.find(trim) == 0)
			s.erase(0, trLen);

		return s;
	}

	template<class charT>
	static string_base<charT> rtrim(string_base<charT> s, const string_base<charT>& trim) {
		size_t trLen = trim.length();

		while (s.length() >= trLen && s.rfind(trim) == s.length() - trLen)
			s.erase(s.length() - trLen);

		return s;
	}

	template<class charT>
	static string_base<charT> rtruncate(const string_base<charT>& str, size_t truncLen) {
		return str.length() > truncLen ? str.substr(str.length() - truncLen) : str;
	}

	template<class charT>
	static vector<string_base<charT>> split(
		const string_base<charT>& str, const charT delim, bool trimWs = false)
	{
		static const string_base<charT> _space = getSpace<charT>();
		vector<string_base<charT>> splits{};
		string_base<charT> subStr;

		for (charT ch : str) {
			if (ch == delim) {
				if (trimWs) subStr = trim<charT>(subStr, _space);
				splits.push_back(subStr);
				subStr.clear();
			}
			else {
				subStr += ch;
			}
		}

		if (trimWs) subStr = trim<charT>(subStr, _space);
		splits.push_back(subStr);
		return splits;
	}

	template<class charT>
	static vector<string_base<charT>> split(
		const string_base<charT>& str, const string_base<charT>& delim, bool trimWs = false)
	{
		static const string_base<charT> _space = getSpace<charT>();
		vector<string_base<charT>> splits;
		size_t start = 0, end = str.find(delim);
		string_base<charT> subStr;

		while (end != string::npos) {
			subStr = str.substr(start, end - start);
			if (trimWs) subStr = trim<charT>(subStr, _space);
			splits.push_back(subStr);

			start = end + delim.length();
			end = str.find(delim, start);
		}

		subStr = str.substr(start);
		if (trimWs) subStr = trim<charT>(subStr, _space);
		splits.push_back(subStr);
		return splits;
	}

	template<class charT>
	static string_base<charT> format(string_base<charT> str,
		const vector<string_base<charT>> pars, const size_t startIndex = 0,
		const pair<string_base<charT>, string_base<charT>>& phWrapper = getDefaultPlaceholderWrapper<charT>())
	{
		string_base<charT> placeholder;

		for (size_t i = 0, j = i + startIndex; i < pars.size(); i++, j++) {
			placeholder = createPlaceholder<charT>(j, phWrapper);
			str = replace<charT>(str, placeholder, pars[i]);
		}

		return str;
	}

	static wstring convertToW(const string& str) {
		int count = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), NULL, 0);
		wstring wstr(count, 0);
		MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), &wstr[0], count);
		return wstr;
	}

	static string convertFromW(const wstring& wstr) {
		int count = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wstr.length(), NULL, 0, NULL, NULL);
		string str(count, 0);
		WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], count, NULL, NULL);
		return str;
	}

private:
	template<class charT>
	static string_base<charT> getSpace() {
		static const string_base<charT> _space(1, (charT)32); // " "
		return _space;
	}

	template<class charT>
	static string_base<charT> getBlank() {
		return string_base<charT>();
	}

	template<class charT>
	static string_base<charT> createPlaceholder(size_t i, 
		const pair<string_base<charT>, string_base<charT>>& phWrapper)
	{
		return phWrapper.first + _Integral_to_string<charT>(i) + phWrapper.second;
	}
};
