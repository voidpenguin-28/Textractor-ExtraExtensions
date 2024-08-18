
#pragma once
#include "FileWriter.h"
#include "../DirectoryCreator.h"
#include <unordered_set>

class DirCreatorFileWriter : public FileWriter {
public:
	DirCreatorFileWriter(FileWriter& mainFileWriter, DirectoryCreator& dirCreator) 
		: _mainFileWriter(mainFileWriter), _dirCreator(dirCreator) { }

	void writeToFile(const string& filePath, const string& text) override {
		createDirIfFileNotSeen(filePath);
		_mainFileWriter.writeToFile(filePath, text);
	}

	void writeToFile(const string& filePath, const vector<string>& lines, size_t startIndex = 0) override {
		createDirIfFileNotSeen(filePath);
		_mainFileWriter.writeToFile(filePath, lines, startIndex);
	}

	void appendToFile(const string& filePath, const string& text) override {
		createDirIfFileNotSeen(filePath);
		_mainFileWriter.appendToFile(filePath, text);
	}

	void appendToFile(const string& filePath, const vector<string>& lines, size_t startIndex = 0) override {
		createDirIfFileNotSeen(filePath);
		_mainFileWriter.appendToFile(filePath, lines, startIndex);
	}
private:
	FileWriter& _mainFileWriter;
	DirectoryCreator& _dirCreator;
	unordered_set<string> _seenFiles{};

	void createDirIfFileNotSeen(const string& filePath) {
		if (fileSeen(filePath)) return;
		_dirCreator.createDir(filePath);
		_seenFiles.insert(filePath);
	}

	bool fileSeen(const string& filePath) {
		return _seenFiles.find(filePath) != _seenFiles.end();
	}
};
