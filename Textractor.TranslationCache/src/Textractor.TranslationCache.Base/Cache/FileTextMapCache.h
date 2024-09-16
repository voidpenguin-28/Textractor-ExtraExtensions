
#pragma once
#include "../_Libraries/Locker.h"
#include "TextMapCache.h"
#include "../File/FileDeleter.h"
#include "../File/FileReader.h"
#include "../File/Writer/FileWriter.h"


class FileTextMapCache : public TextMapCache {
public:
	FileTextMapCache(FileWriter& fileWriter, FileReader& fileReader, FileDeleter& fileDeleter, 
		const TextFormatter& formatter, const TextMapper& textMapper, const function<string()>& cacheFilePathGetter)
		: _fileWriter(fileWriter), _fileReader(fileReader), _fileDeleter(fileDeleter), _formatter(formatter),
			_textMapper(textMapper), _cacheFilePathGetter(cacheFilePathGetter) { }
	FileTextMapCache(FileWriter& fileWriter, FileReader& fileReader, FileDeleter& fileDeleter,
		const TextFormatter& formatter, const TextMapper& textMapper, const string& cacheFilePath)
		: FileTextMapCache(fileWriter, fileReader, fileDeleter, formatter, textMapper, 
			[cacheFilePath]() { return cacheFilePath; }) { }

	bool keyExists(const wstring& key) const override {
		return !readFromCache(key).empty();
	}

	wstring readFromCache(const wstring& key) const override {
		wstring formattedKey = _formatter.format(key);
		string filePath = _cacheFilePathGetter();
		pair<wstring, wstring> textPair;
		_writeLocker.waitForUnlock();

		string line = _fileReader.readLine(filePath, [this, &formattedKey, &textPair](const string& line) {
			textPair = importFormatToPair(line);
			return textPair.first == formattedKey;
		});

		return !line.empty() ? textPair.second : L"";
	}

	unordered_map<wstring, wstring> readAllFromCache() const override {
		string filePath = _cacheFilePathGetter();
		unordered_map<wstring, wstring> cache{};
		_writeLocker.waitForUnlock();

		_fileReader.readLines(filePath, [this, &cache](const string& line) {
			pair<wstring, wstring> textPair = importFormatToPair(line);
			cache[textPair.first] = textPair.second;
		});

		return cache;
	}

	void writeToCache(const wstring& key, const wstring& value) override {
		string filePath = _cacheFilePathGetter();
		string output = exportFormat(key, value);

		_writeLocker.lock([this, &filePath, &output]() {
			appendToFile(filePath, output);
		});
	}

	void writeAllToCache(const unordered_map<wstring, wstring> cache, bool reload = false) override {
		string filePath = _cacheFilePathGetter();
		vector<string> lines = exportFormat(cache);

		_writeLocker.lock([this, &filePath, &lines, reload]() {
			if (reload) writeToFile(filePath, lines);
			else appendToFile(filePath, lines);
		});
	}

	void removeFromCache(const wstring& key) override {
		string filePath = _cacheFilePathGetter();
		wstring formattedKey = _formatter.format(key);

		_writeLocker.lock([this, &filePath, &formattedKey]() {
			deleteFile(filePath);

			_fileReader.readLines(filePath, [this, &filePath, &formattedKey](const string& line) {
				pair<wstring, wstring> textPair = importFormatToPair(line);
				if (formattedKey == textPair.first) return;
				appendToFile(filePath, line);
			});
		});
	}

	void clearCache() override {
		string cacheFilePath = _cacheFilePathGetter();

		_writeLocker.lock([this, &cacheFilePath]() {
			deleteFile(cacheFilePath);
		});
	}
private:
	const function<string()> _cacheFilePathGetter;
	FileWriter& _fileWriter;
	FileReader& _fileReader;
	FileDeleter& _fileDeleter;
	const TextFormatter& _formatter;
	const TextMapper& _textMapper;
	mutable BasicLocker _writeLocker;
	const vector<pair<wstring, wstring>> _charMappings = {
		pair<wstring, wstring>(L"\r", L"\\r"),
		pair<wstring, wstring>(L"\n", L"\\n"),
	};

	void writeToFile(const string& filePath, const vector<string>& lines) {
		_fileWriter.writeToFile(filePath, lines);
	}

	void appendToFile(const string& filePath, const string& text) {
		_fileWriter.appendToFile(filePath, text);
	}

	void appendToFile(const string& filePath, const vector<string>& lines) {
		_fileWriter.appendToFile(filePath, lines);
	}

	void deleteFile(const string& filePath) {
		_fileDeleter.deleteFile(filePath);
	}

	pair<wstring, wstring> importFormatToPair(string text) const {
		return importFormatToPair(StrHelper::convertToW(text));
	}

	pair<wstring, wstring> importFormatToPair(wstring text) const {
		wstring wText = importFormat(text);
		return _textMapper.split(wText);
	}

	wstring importFormat(wstring text) const {
		for (const pair<wstring, wstring>& mapping : _charMappings) {
			text = StrHelper::replace<wchar_t>(text, mapping.second, mapping.first);
		}

		return text;
	}

	vector<string> exportFormat(const unordered_map<wstring, wstring> cache) const {
		vector<string> lines{};

		for (const auto& item : cache) {
			lines.push_back(exportFormat(item.first, item.second));
		}

		return lines;
	}

	string exportFormat(const wstring& key, const wstring& value) const {
		wstring fullText = _textMapper.merge(_formatter.format(key), value);
		return exportFormat(fullText);
	}

	string exportFormat(wstring text) const {
		for (const pair<wstring, wstring>& mapping : _charMappings) {
			text = StrHelper::replace<wchar_t>(text, mapping.first, mapping.second);
		}

		return StrHelper::convertFromW(text);
	}
};
