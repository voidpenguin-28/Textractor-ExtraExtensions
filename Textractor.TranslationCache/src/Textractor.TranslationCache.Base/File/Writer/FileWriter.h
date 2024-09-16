
#pragma once
#include <string>
#include <vector>
using namespace std;


class FileWriter {
public:
	virtual ~FileWriter() { }
	virtual void writeToFile(const string& filePath, const string& text) = 0;
	virtual void writeToFile(const string& filePath, const vector<string>& lines, size_t startIndex = 0) = 0;
	virtual void appendToFile(const string& filePath, const string& text) = 0;
	virtual void appendToFile(const string& filePath, const vector<string>& lines, size_t startIndex = 0) = 0;
};


class NoFileWriter : public FileWriter {
public:
	void writeToFile(const string& filePath, const string& text) override { }
	void writeToFile(const string& filePath, const vector<string>& lines, size_t startIndex = 0) override { }
	void appendToFile(const string& filePath, const string& text) override { }
	void appendToFile(const string& filePath, const vector<string>& lines, size_t startIndex = 0) override { }
};

