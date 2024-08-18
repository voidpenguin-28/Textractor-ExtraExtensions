
#pragma once
#include <fstream>
#include <functional>
#include <string>
#include <unordered_set>
#include <vector>
#include "../Libraries/Locker.h"
#include "FileReader.h"
#include "Writer/FileWriter.h"
using namespace std;


class FileTruncater {
public:
	virtual ~FileTruncater() { }
	virtual void truncateFile(const string& filePath) = 0;
};


class NoFileTruncater : public FileTruncater {
public:
	void truncateFile(const string& filePath) override {
		return;
	}
};


class DefaultFileTruncater : public FileTruncater {
public:
	DefaultFileTruncater(FileReader& fileReader, FileWriter& fileWriter, 
		const function<uint64_t()>& sizeLimitBytesGetter)
		: _fileReader(fileReader), _fileWriter(fileWriter), _sizeLimitBytesGetter(sizeLimitBytesGetter) { }
	DefaultFileTruncater(FileReader& fileReader, FileWriter& fileWriter, const uint64_t sizeLimitBytes)
		: DefaultFileTruncater(fileReader, fileWriter, [sizeLimitBytes]() { return sizeLimitBytes; }) { }

	void truncateFile(const string& filePath) override {
		_lockerMap.getOrCreateLocker(filePath).lock([this, &filePath]() {
			truncateFileBase(filePath);
		});
	}
private:
	DefaultLockerMap<string> _lockerMap;
	FileReader& _fileReader;
	FileWriter& _fileWriter;
	const function<uint64_t()> _sizeLimitBytesGetter;
	

	void truncateFileBase(const string& filePath) {
		uint64_t fileSizeBytes = getFileSize(filePath);
		if (fileSizeBytes < _sizeLimitBytesGetter()) return;

		double sizeRatio = getSizeRatio(fileSizeBytes);
		pair<vector<string>, long> contents = readAllLines(filePath);
		vector<string> lines = contents.first;

		size_t startIndex = getTruncStartIndex(lines, contents.second, sizeRatio);
		_fileWriter.writeToFile(filePath, lines, startIndex);
	}

	uint64_t getFileSize(const string& filePath) {
		HANDLE hFile = getFileHandle(StrHelper::convertToW(filePath).c_str());
		if (hFile == INVALID_HANDLE_VALUE) return 0;

		DWORD fileSize = GetFileSize(hFile, NULL);
		CloseHandle(hFile);
		return static_cast<uint64_t>(fileSize);
	}

	HANDLE getFileHandle(const wchar_t* filePath) {
		return CreateFile(filePath, GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
	}

	double getSizeRatio(uint64_t fileSizeBytes) {
		return _sizeLimitBytesGetter() / static_cast<double>(fileSizeBytes);
	}

	size_t getTruncStartIndex(const vector<string>& lines, long linesTotalLength, double sizeRatio) {
		long maxLength = static_cast<long>(linesTotalLength * sizeRatio);
		long currLength = linesTotalLength;
		size_t index = 0;

		while (currLength > maxLength && index < lines.size()) {
			currLength -= lines[index++].length();
		}

		return index;
	}

	pair<vector<string>, long> readAllLines(const string& filePath) {
		vector<string> lines{};
		long length = 0;
		
		_fileReader.readLines(filePath, [&lines, &length](const string& line) {
			lines.push_back(line);
			length += line.length();
		});

		return pair<vector<string>, long>(lines, length);
	}
};
