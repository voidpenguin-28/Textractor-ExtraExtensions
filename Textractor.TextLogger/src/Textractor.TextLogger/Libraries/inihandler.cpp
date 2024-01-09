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
	return getMatch(line, _sectionPattern, 1);
}

wstring IniParser::extractKeyName(const wstring& line) const {
	return getMatch(line, _keyValPattern, 1);
}

wstring IniParser::extractKeyValue(const wstring& line) const {
	return getMatch(line, _keyValPattern, 2);
}

wstring IniParser::formatSection(wstring section) const {
	section = trimWhitespace(section);
	if (section.empty()) return section;

	if (section.find(SECT_START_CH) == wstring::npos)
		section = SECT_START_CH + section + SECT_END_CH;

	return section;
}


// *** PRIVATE IniParser

const wstring IniParser::SECT_START_CH = L"[";
const wstring IniParser::SECT_END_CH = L"]";
const wstring IniParser::MATCH_ANY = L"*";
const wregex IniParser::_sectionPattern = wregex(L"^\\s{0,}(\\[.+\\])\\s{0,}$");
const wregex IniParser::_keyValPattern = wregex(L"^\\s{0,}(.+)\\s{0,}=\\s{0,}(.{0,})\\s{0,}$");


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

wstring IniParser::getMatch(const wstring& str, const wregex& pattern, size_t matchIndex) const {
	wsmatch matches;

	return regex_match(str, matches, pattern) ? matches[matchIndex].str() : L"";
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

vector<pair<wstring, wstring>> IniContents::getAllValues(const wstring& section) const {
	lock_guard<mutex> lock(_mutex);
	return _getAllValues(section);
}

bool IniContents::setValue(const wstring& section, const wstring& key, wstring value, bool overrideIfExists) {
	lock_guard<mutex> lock(_mutex);
	return _setValue(section, key, value, overrideIfExists);
}

bool IniContents::setValue(const wstring& section, const wstring& key, int value, bool overrideIfExists)
{
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

const vector<pair<wstring, wstring>> IniContents::_formatPairs = {
	pair<wstring, wstring>(L"\\r", L"\r"),
	pair<wstring, wstring>(L"\\n", L"\n"),
	pair<wstring, wstring>(L"\\t", L"\t"),
	pair<wstring, wstring>(L"\\\\", L"\\")
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
	for (auto& format : _formatPairs) {
		value = replace(value, format.first, format.second);
	}

	return value;
}

wstring IniContents::formatWriteKeyValue(wstring value) const {
	for (auto& format : _formatPairs) {
		value = replace(value, format.second, format.first);
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

	if(!_iniLines[_iniLines.size() - 1].empty()) _iniLines.push_back(L"");
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
	lock_guard<mutex> lock(_mutex);
	return readIni_();
}

void IniFileHandler::saveIni(IniContents& content, const string& newFilePath) const {
	lock_guard<mutex> lock(_mutex);
	_saveIni(content);
}


// *** PRIVATE IniFileHandler

const wregex IniFileHandler::_lineDelimPattern(L"(?:\r\n|\r|\n)");

void IniFileHandler::_saveIni(IniContents& content, const string& newFilePath) const {
	wstring fileContents = content.stringCopy();
	_saveIni(fileContents, newFilePath);
}

void IniFileHandler::_saveIni(const wstring& content, const string& newFilePath) const {
	string filePath = !newFilePath.empty() ? newFilePath : _iniFilePath;
	ofstream f(filePath);
	if (!f.is_open()) throw runtime_error("Could not open ini file: " + filePath);

	f << convertFromW(content);
	f.close();
}

IniContents* IniFileHandler::readIni_() const {
	vector<wstring> lines = getIniFileContents();
	return new IniContents(lines);
}

vector<wstring> IniFileHandler::getIniFileContents() const {
	ifstream f(_iniFilePath);
	if (!f.is_open()) {
		f.close();
		_saveIni(L"");
		f = ifstream(_iniFilePath);

		if(!f.is_open())
			throw runtime_error("Could not open ini file: " + _iniFilePath);
	}

	stringstream buffer;
	buffer << f.rdbuf();
	f.close();

	wstring contents = convertToW(buffer.str());
	vector<wstring> lines = splitLines(contents);
	return lines;
}

vector<wstring> IniFileHandler::splitLines(const wstring& text) const {
	wsregex_token_iterator iterator(text.begin(), text.end(), _lineDelimPattern, -1);
	wsregex_token_iterator end;
	vector<wstring> lines;

	while (iterator != end) {
		lines.push_back(*iterator);
		++iterator;
	}

	return lines;
}

wstring IniFileHandler::convertToW(const string& str) const {
	return wstring_convert<codecvt_utf8_utf16<wchar_t>>().from_bytes(str);
}

string IniFileHandler::convertFromW(const wstring& str) const {
	return wstring_convert<codecvt_utf8_utf16<wchar_t>>().to_bytes(str);
}
