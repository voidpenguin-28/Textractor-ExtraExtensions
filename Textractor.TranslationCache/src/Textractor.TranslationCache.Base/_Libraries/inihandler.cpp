#pragma once

#include "inihandler.h"
#include <sstream>


// *** PUBLIC IniParser

size_t IniParser::findKeyIndex(const vector<wstring>& lines, const wstring& section, const wstring& key) const {
	size_t sectIndex = findSectionIndex(lines, section);
	if (sectIndex == wstring::npos) return wstring::npos;

	size_t nextSectIndex = findNextSection(lines, sectIndex + 1);
	size_t keyIndex = findLineContaining(lines, key, _keyNameParser, sectIndex, nextSectIndex);
	return keyIndex;
}

size_t IniParser::findSectionIndex(const vector<wstring>& lines, const wstring& section) const {
	wstring formattedSection = formatSection(section);
	return findLineContaining(lines, formattedSection, _sectionParser, 0);
}

size_t IniParser::findNextSection(const vector<wstring>& lines, size_t lineStart) const {
	return findLineContaining(lines, MATCH_ANY, _sectionParser, lineStart);
}


wstring IniParser::extractSectionName(const wstring& line) const {
	wstring section = trimWhitespace(line);
	if (line.empty()) return L"";

	return section[0] == SECT_START_CH && section.back() == SECT_END_CH ? section : L"";
}

wstring IniParser::extractKeyName(const wstring& line) const {
	size_t delimIndex = line.find(KEY_VAL_DELIM);
	if (delimIndex == wstring::npos) return L"";

	return trimWhitespace(line.substr(0, delimIndex));
}

wstring IniParser::extractKeyValue(const wstring& line) const {
	size_t delimIndex = line.find(KEY_VAL_DELIM);
	if (delimIndex == wstring::npos) return L"";

	return trimWhitespace(line.substr(delimIndex + 1));
}

wstring IniParser::formatSection(wstring section) const {
	section = trimWhitespace(section);
	if (section.empty()) return section;

	if (section.find(SECT_START_CH) == wstring::npos)
		section = SECT_START_CH + section + SECT_END_CH;

	return section;
}


// *** PRIVATE IniParser

const wstring IniParser::MATCH_ANY = L"*";

wstring IniParser::trimWhitespace(const wstring& str) const {
	size_t first = str.find_first_not_of(L" \t\n\r");
	size_t last = str.find_last_not_of(L" \t\n\r");

	if (first == std::string::npos || last == std::string::npos) return L"";
	return str.substr(first, last - first + 1);
}

// if 'match = MATCH_ANY', then match on any non-empty string returned by line parser
size_t IniParser::findLineContaining(const vector<wstring>& lines, const wstring& match,
	const function<wstring(const wstring&)>& lineParser, size_t lineStart, size_t lineEnd) const
{
	if (lineEnd > lines.size()) lineEnd = lines.size();
	wstring parsed;

	for (size_t i = lineStart; i < lineEnd; i++) {
		parsed = lineParser(lines[i]);

		if (match == MATCH_ANY && !parsed.empty()) return i;
		if (parsed == match) return i;
	}

	return string::npos;
}


//// *** PUBLIC IniContents

bool IniContents::sectionExists(const wstring& section) const {
	lock_guard<mutex> lock(_mutex);
	return _sectionExists(section);
}

bool IniContents::keyExists(const wstring& section, const wstring& key) const {
	lock_guard<mutex> lock(_mutex);
	return _keyExists(section, key);
}

wstring IniContents::stringCopy() const {
	lock_guard<mutex> lock(_mutex);
	return _stringCopy();
}

wstring IniContents::getValue(const wstring& section, const wstring& key, wstring defaultValue) const {
	lock_guard<mutex> lock(_mutex);
	return _getValue(section, key, defaultValue);
}

int IniContents::getValue(const wstring& section, const wstring& key, int defaultValue) const {
	wstring value = getValue(section, key, to_wstring(defaultValue));
	return stoi(value);
}

double IniContents::getValue(const wstring& section, const wstring& key, double defaultValue) const {
	wstring value = getValue(section, key, to_wstring(defaultValue));
	return stod(value);
}

vector<pair<wstring, wstring>> IniContents::getAllValues(const wstring& section) const {
	lock_guard<mutex> lock(_mutex);
	return _getAllValues(section);
}

bool IniContents::setValue(const wstring& section, const wstring& key, wstring value, bool overrideIfExists) {
	lock_guard<mutex> lock(_mutex);
	return _setValue(section, key, value, overrideIfExists);
}

bool IniContents::setValue(const wstring& section, const wstring& key, int value, bool overrideIfExists) {
	return setValue(section, key, to_wstring(value), overrideIfExists);
}

bool IniContents::setValue(const wstring& section, const wstring& key, double value, bool overrideIfExists) {
	return setValue(section, key, to_wstring(value), overrideIfExists);
}

bool IniContents::removeValue(const wstring& section, const wstring& key) {
	lock_guard<mutex> lock(_mutex);
	return _removeValue(section, key);
}

bool IniContents::removeSection(const wstring& section) {
	lock_guard<mutex> lock(_mutex);
	return _removeSection(section);
}

// *** PRIVATE IniContents

const IniParser IniContents::_iniParser{};

const unordered_map<wchar_t, wchar_t> IniContents::_escapePairs = {
	{ L'\\', L'\\' },
	{ L'r', L'\r' },
	{ L'n', L'\n' },
	{ L't', L'\t' },
	{ L'"', L'"' },
};

const vector<pair<wstring, wstring>> IniContents::_formatPairs2 = {
	pair<wstring, wstring>(L"\\", L"\\\\"),
	pair<wstring, wstring>(L"\r", L"\\r"),
	pair<wstring, wstring>(L"\n", L"\\n"),
	pair<wstring, wstring>(L"\t", L"\\t"),
};


bool IniContents::_sectionExists(const wstring& section) const {
	size_t sectionIndex = _iniParser.findSectionIndex(_iniLines, section);
	return indexValid(sectionIndex);
}

bool IniContents::_keyExists(const wstring& section, const wstring& key) const {
	size_t keyIndex = _iniParser.findKeyIndex(_iniLines, section, key);
	return indexValid(keyIndex);
}

wstring IniContents::_stringCopy() const {
	static const wstring LINE_END = L"\n";
	wstring content = L"";

	for (size_t i = 0; i < _iniLines.size(); i++) {
		content += _iniLines[i] + LINE_END;
	}

	if (!content.empty()) content = content.substr(0, content.length() - LINE_END.length());
	return content;
}

wstring IniContents::_getValue(const wstring& section, const wstring& key, wstring defaultValue) const {
	size_t keyIndex = _iniParser.findKeyIndex(_iniLines, section, key);
	if (!indexValid(keyIndex)) return defaultValue;

	wstring value = _iniParser.extractKeyValue(_iniLines[keyIndex]);
	return formatReadKeyValue(value);
}

vector<pair<wstring, wstring>> IniContents::_getAllValues(const wstring& section) const {
	vector<pair<wstring, wstring>> vals = { };
	size_t sectionIndex = _iniParser.findSectionIndex(_iniLines, section);
	if (sectionIndex == wstring::npos) return vals;

	size_t nextSectIndex = _iniParser.findNextSection(_iniLines, sectionIndex + 1);
	if (nextSectIndex == wstring::npos) nextSectIndex = _iniLines.size();
	wstring key, value;

	for (size_t i = sectionIndex + 1; i < nextSectIndex; i++) {
		if (_iniLines[i].empty()) continue;
		key = _iniParser.extractKeyName(_iniLines[i]);
		value = _iniParser.extractKeyValue(_iniLines[i]);

		vals.push_back(pair<wstring, wstring>(key, value));
	}

	return vals;
}

bool IniContents::_setValue(const wstring& section, const wstring& key, wstring value, bool overrideIfExists) {
	size_t keyIndex = _iniParser.findKeyIndex(_iniLines, section, key);
	value = formatWriteKeyValue(value);

	if (!indexValid(keyIndex)) {
		addNewKeyValue(section, key, value);
	}
	else {
		if (!overrideIfExists) return false;
		updateKeyValue(keyIndex, value);
	}

	return true;
}

bool IniContents::_removeValue(const wstring& section, const wstring& key) {
	size_t keyIndex = _iniParser.findKeyIndex(_iniLines, section, key);
	if (keyIndex == wstring::npos) return false;

	_iniLines.erase(_iniLines.begin() + keyIndex);
	return true;
}

bool IniContents::_removeSection(const wstring& section) {
	size_t sectionIndex = _iniParser.findSectionIndex(_iniLines, section);
	if (sectionIndex == wstring::npos) return false;

	size_t nextSectIndex = _iniParser.findNextSection(_iniLines, sectionIndex + 1);
	if (nextSectIndex == wstring::npos) nextSectIndex = _iniLines.size();

	_iniLines.erase(_iniLines.begin() + sectionIndex, _iniLines.begin() + nextSectIndex);
	return true;
}

bool IniContents::indexValid(size_t index) const {
	return index != wstring::npos;
}

wstring IniContents::formatReadKeyValue(wstring value) const {
	static constexpr wchar_t escapeCh = L'\\';
	if (value.empty()) return value;
	wchar_t nextCh;

	for (size_t i = 0; i < value.size() - 1; i++) {
		if (value[i] != escapeCh) continue;
		nextCh = value[i + 1];

		if (nextCh == L'x') {
			value = decodeEscapedUTF8Str(value);
		}
		else if (_escapePairs.find(nextCh) != _escapePairs.end()) {
			replaceAndRemoveCh(value, i, _escapePairs.at(nextCh));
		}
	}

	return value;
}

unsigned char IniContents::hexToByte(const string& hex) const {
	unsigned int value = 0;
	stringstream ss;
	ss << std::hex << hex;
	ss >> value;
	return static_cast<unsigned char>(value);
}

wstring IniContents::decodeEscapedUTF8Str(const wstring& escaped) const {
	try {
		return StrConverter::convertToW(
			decodeEscapedUTF8Str(StrConverter::convertFromW(escaped)));
	}
	catch (exception&) {
		return escaped;
	}
}

string IniContents::decodeEscapedUTF8Str(const string& escaped) const {
	string result;

	for (size_t i = 0; i < escaped.length(); ) {
		if (escaped[i] == L'\\' && i + 3 < escaped.length() && escaped[i + 1] == L'x') {
			string hex = escaped.substr(i + 2, 2);
			result += hexToByte(hex);
			i += 4;
		}
		else {
			result += escaped[i];
			++i;
		}
	}

	return result;
}

void IniContents::replaceAndRemoveCh(wstring& value, size_t startIndex, wchar_t replaceCh) const {
	value.erase(startIndex + 1, 1);
	value[startIndex] = replaceCh;
}

wstring IniContents::formatWriteKeyValue(wstring value) const {
	for (auto& format : _formatPairs2) {
		value = replace(value, format.first, format.second);
	}

	return value;
}

void IniContents::addNewKeyValue(const wstring& section, const wstring& key, const wstring& value) {
	size_t sectionIndex = createSectionIfNotExist(section);
	wstring keyValLine = key + L"=" + value;
	_iniLines.insert(_iniLines.begin() + sectionIndex + 1, keyValLine);
}

void IniContents::updateKeyValue(size_t keyIndex, const wstring& value) {
	if (!indexValid(keyIndex)) return;

	wstring line = _iniLines[keyIndex];
	wstring currVal = _iniParser.extractKeyValue(line);
	size_t currValIndex = line.rfind(currVal);
	_iniLines[keyIndex] = replace(line, currVal, value);
}

size_t IniContents::createSectionIfNotExist(const wstring& section) {
	size_t sectionIndex = _iniParser.findSectionIndex(_iniLines, section);
	if (indexValid(sectionIndex)) return sectionIndex;

	if (!_iniLines[_iniLines.size() - 1].empty()) _iniLines.push_back(L"");
	_iniLines.push_back(_iniParser.formatSection(section));
	return _iniLines.size() - 1;
}

wstring IniContents::replace(const wstring& input, const wstring& target, const wstring& replacement) const {
	wstring result = input;
	size_t startPos = 0;

	while (indexValid(startPos = result.find(target, startPos))) {
		result.replace(startPos, target.length(), replacement);
		startPos += replacement.length();
	}

	return result;
}


//// *** PUBLIC IniFileHandler

IniContents* IniFileHandler::readIni() const {
	string contentsStr;

	{
		lock_guard<mutex> lock(_mutex);
		contentsStr = getIniFileContents();
	}

	return parseIniFileContents(contentsStr);
}

void IniFileHandler::saveIni(IniContents& content, const string& newFilePath) const {
	string contentStr = StrConverter::convertFromW(content.stringCopy());
	string filePath = !newFilePath.empty() ? newFilePath : _iniFilePath;

	{
		lock_guard<mutex> lock(_mutex);
		saveIniFileContents(contentStr, filePath);
	}
}


// *** PRIVATE IniFileHandler

void IniFileHandler::saveIniFileContents(const string& content, const string& filePath) const {
	ofstream f(filePath);
	if (!f.is_open()) throw runtime_error("Could not open ini file: " + filePath);

	f << content;
	f.close();
}

string IniFileHandler::getIniFileContents() const {
	ifstream f(_iniFilePath);

	if (!f.is_open()) {
		f.close();
		saveIniFileContents("", _iniFilePath); //create blank ini file if not exists
		f = ifstream(_iniFilePath);

		if (!f.is_open())
			throw runtime_error("Could not open ini file: " + _iniFilePath);
	}

	stringstream buffer;
	buffer << f.rdbuf();
	f.close();
	return buffer.str();
}

IniContents* IniFileHandler::parseIniFileContents(const string& fileContents) const {
	wstring contentsW = StrConverter::convertToW(fileContents);
	vector<wstring> lines = splitLines(contentsW);
	return new IniContents(lines);
}

vector<wstring> IniFileHandler::splitLines(const wstring& text) const {
	static constexpr wchar_t CRG_RTN = L'\r';
	static constexpr wchar_t NW_LN = L'\n';

	vector<wstring> lines{};
	wstring line = L"";

	for (size_t i = 0; i < text.length(); i++) {
		if (text[i] == CRG_RTN) {
			lines.push_back(line);
			line = L"";
		}
		else if (text[i] == NW_LN) {
			if (i > 0 && text[i - 1] == CRG_RTN) continue;
			lines.push_back(line);
			line = L"";
		}
		else {
			line += text[i];
		}
	}

	lines.push_back(line);
	return lines;
}
