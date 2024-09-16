
#pragma once
#include "FileWriter.h"
#include "../FileTruncater.h"


class TruncatableFileWriter : public FileWriter {
public:
	TruncatableFileWriter(FileWriter& mainWriter, FileTruncater& truncater)
		: _mainWriter(mainWriter), _truncater(truncater) { }

	void writeToFile(const string& filePath, const string& text) override {
		_lockerMap.getOrCreateLocker(filePath).lock([this, &filePath, &text]() {
			_mainWriter.writeToFile(filePath, text);
			_truncater.truncateFile(filePath);
		});
	}

	void writeToFile(const string& filePath, const vector<string>& lines, size_t startIndex = 0) override {
		_lockerMap.getOrCreateLocker(filePath).lock([this, &filePath, &lines, startIndex]() {
			_mainWriter.writeToFile(filePath, lines, startIndex);
			_truncater.truncateFile(filePath);
		});
	}

	void appendToFile(const string& filePath, const string& text) override {
		_lockerMap.getOrCreateLocker(filePath).lock([this, &filePath, &text]() {
			_mainWriter.appendToFile(filePath, text);
			_truncater.truncateFile(filePath);
		});
	}

	void appendToFile(const string& filePath, const vector<string>& lines, size_t startIndex = 0) override {
		_lockerMap.getOrCreateLocker(filePath).lock([this, &filePath, &lines, startIndex]() {
			_mainWriter.appendToFile(filePath, lines, startIndex);
			_truncater.truncateFile(filePath);
		});
	}
private:
	DefaultLockerMap<string> _lockerMap;
	FileWriter& _mainWriter;
	FileTruncater& _truncater;
};
