#pragma once
#include <codecvt>
#include <string>
using namespace std;

inline u32string convertToU32(string str) {
	return wstring_convert<codecvt_utf8<char32_t>, char32_t >().from_bytes(str);
}

inline string convertFromU32(u32string str) {
	return wstring_convert<codecvt_utf8<char32_t>, char32_t >().to_bytes(str);
}

inline wstring convertToW(string str) {
	return wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().from_bytes(str);
}

inline string convertFromW(wstring str) {
	return wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().to_bytes(str);
}
