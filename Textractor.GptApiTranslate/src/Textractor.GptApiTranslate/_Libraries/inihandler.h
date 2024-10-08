#pragma once

#include <iostream>
#include <fstream>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <windows.h>
using namespace std;


class IniParser {
public:
	size_t findKeyIndex(const vector<wstring>& lines, const wstring& section, const wstring& key) const;
	size_t findSectionIndex(const vector<wstring>& lines, const wstring& section) const;
	size_t findNextSection(const vector<wstring>& lines, size_t lineStart) const;

	wstring extractSectionName(const wstring& line) const;
	wstring extractKeyName(const wstring& line) const;
	wstring extractKeyValue(const wstring& line) const;
	wstring formatSection(wstring section) const;
private:
	static constexpr wchar_t SECT_START_CH = L'[', SECT_END_CH = L']', KEY_VAL_DELIM = L'=';
	static const wstring MATCH_ANY;

	const function<wstring(const wstring&)> _sectionParser = [this](const wstring& s) { return extractSectionName(s); };
	const function<wstring(const wstring&)> _keyNameParser = [this](const wstring& s) { return extractKeyName(s); };
	const function<wstring(const wstring&)> _keyValueParser = [this](const wstring& s) { return extractKeyValue(s); };

	wstring trimWhitespace(const wstring& str) const;
	size_t findLineContaining(const vector<wstring>& lines, const wstring& match,
		const function<wstring(const wstring&)>& lineParser, size_t lineStart = 0, size_t lineEnd = UINT_MAX) const;
};


class IniContents {
public:
	IniContents(const vector<wstring>& iniLines) : _iniLines(iniLines) { }

	bool sectionExists(const wstring& section) const;
	bool keyExists(const wstring& section, const wstring& key) const;
	wstring stringCopy() const;
	wstring getValue(const wstring& section, const wstring& key, wstring defaultValue = L"") const;
	int getValue(const wstring& section, const wstring& key, int defaultValue) const;
	double getValue(const wstring& section, const wstring& key, double defaultValue) const;
	vector<pair<wstring, wstring>> getAllValues(const wstring& section) const;
	bool setValue(const wstring& section, const wstring& key, wstring value, bool overrideIfExists = true);
	bool setValue(const wstring& section, const wstring& key, int value, bool overrideIfExists = true);
	bool setValue(const wstring& section, const wstring& key, double value, bool overrideIfExists = true);
	bool removeValue(const wstring& section, const wstring& key);
	bool removeSection(const wstring& section);
private:
	static const IniParser _iniParser;
	static const unordered_map<wchar_t, wchar_t> _escapePairs;
	static const vector<pair<wstring, wstring>> _formatPairs2;

	vector<wstring> _iniLines;
	mutable mutex _mutex;

	bool _sectionExists(const wstring& section) const;
	bool _keyExists(const wstring& section, const wstring& key) const;
	wstring _stringCopy() const;

	wstring _getValue(const wstring& section, const wstring& key, wstring defaultValue = L"") const;
	vector<pair<wstring, wstring>> _getAllValues(const wstring& section) const;
	bool _setValue(const wstring& section, const wstring& key, wstring value, bool overrideIfExists = true);
	bool _removeValue(const wstring& section, const wstring& key);
	bool _removeSection(const wstring& section);

	bool indexValid(size_t index) const;
	wstring formatReadKeyValue(wstring value) const;
	unsigned char hexToByte(const string& hex) const;
	wstring decodeEscapedUTF8Str(const wstring& escaped) const;
	string decodeEscapedUTF8Str(const string& escaped) const;
	void replaceAndRemoveCh(wstring& value, size_t startIndex, wchar_t replaceCh) const;
	wstring formatWriteKeyValue(wstring value) const;
	void addNewKeyValue(const wstring& section, const wstring& key, const wstring& value);
	void updateKeyValue(size_t keyIndex, const wstring& value);

	size_t createSectionIfNotExist(const wstring& section);
	wstring replace(const wstring& input, const wstring& target, const wstring& replacement) const;
};


class IniFileHandler {
public:
	IniFileHandler(const string& iniFilePath) : _iniFilePath(iniFilePath) { }
	
	IniContents* readIni() const;
	void saveIni(IniContents& content, const string& newFilePath = "") const;
private:
	const string _iniFilePath;
	mutable mutex _mutex;

	void saveIniFileContents(const string& content, const string& newFilePath = "") const;
	string getIniFileContents() const;
	IniContents* parseIniFileContents(const string& fileContents) const;
	vector<wstring> splitLines(const wstring& text) const;
};

class StrConverter {
public:
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
};

