
#pragma once
#include "../../Libraries/Locker.h"
#include "FileWriter.h"
#include <fstream>
#include <memory>
#include <unordered_map>


class FstreamFileWriterBase : public FileWriter {
public:
	virtual ~FstreamFileWriterBase() { }

	void writeToFile(const string& filePath, const string& text) override {
		writeToFileThreadSafe(filePath, text, false);
	}

	void writeToFile(const string& filePath, const vector<string>& lines, size_t startIndex = 0) override {
		writeToFileThreadSafe(filePath, lines, false, startIndex);
	}

	void appendToFile(const string& filePath, const string& text) override {
		writeToFileThreadSafe(filePath, text, true);
	}

	void appendToFile(const string& filePath, const vector<string>& lines, size_t startIndex = 0) override {
		writeToFileThreadSafe(filePath, lines, true, startIndex);
	}
protected:
	DefaultLockerMap<string> _lockerMap;

	virtual void writeToFileBase(const string& filePath, const string& text, bool append) = 0;
	virtual void writeToFileBase(const string& filePath, 
		const vector<string>& lines, bool append, size_t startIndex = 0) = 0;

	void writeToFileThreadSafe(const string& filePath, const string& text, bool append) {
		_lockerMap.getOrCreateLocker(filePath).lock([this, &filePath, &text, append]() {
			writeToFileBase(filePath, text, append);
		});
	}

	void writeToFileThreadSafe(const string& filePath, const vector<string>& lines, bool append, size_t startIndex = 0) {
		_lockerMap.getOrCreateLocker(filePath).lock([this, &filePath, &lines, append, startIndex]() {
			writeToFileBase(filePath, lines, append, startIndex);
		});
	}

	virtual void writeToFile(ofstream& f, const string& filePath, const string& text) {
		f << text << endl;
	}

	virtual void writeToFile(ofstream& f, const string& filePath, 
		const vector<string>& lines, size_t startIndex = 0)
	{
		for (size_t i = startIndex; i < lines.size(); i++) f << lines[i] << '\n';
		f << flush;
	}

	virtual ios_base::open_mode getOpenMode(bool append) {
		return append ? ios_base::app : ios_base::out;
	}
};


class FstreamFileWriter : public FstreamFileWriterBase {
protected:
	void writeToFileBase(const string& filePath, const string& text, bool append) override {
		ofstream f(filePath, getOpenMode(append));
		FstreamFileWriterBase::writeToFile(f, filePath, text);
	}

	void writeToFileBase(const string& filePath, const vector<string>& lines, 
		bool append, size_t startIndex = 0) override 
	{
		ofstream f(filePath, getOpenMode(append));
		FstreamFileWriterBase::writeToFile(f, filePath, lines, startIndex);
	}
};


// once a file is opened, it stays open unless 'closeAll' or destructor is called
class PersistentFstreamFileWriter : public FstreamFileWriterBase {
public:
	virtual ~PersistentFstreamFileWriter() {
		closeAll();
	}

	void closeAll() {
		_fileMap.clear();
		_fileModeMap.clear();
	}
protected:
	void writeToFileBase(const string& filePath, const string& text, bool append) override {
		ofstream& f = getOrCreateStream(filePath, FstreamFileWriterBase::getOpenMode(append));
		FstreamFileWriterBase::writeToFile(f, filePath, text);
	}

	void writeToFileBase(const string& filePath, const vector<string>& lines, 
		bool append, size_t startIndex = 0) override
	{
		ofstream& f = getOrCreateStream(filePath, FstreamFileWriterBase::getOpenMode(append));
		FstreamFileWriterBase::writeToFile(f, filePath, lines, startIndex);
	}
private:
	unordered_map<string, unique_ptr<ofstream>> _fileMap;
	unordered_map<string, int> _fileModeMap;

	ofstream& getOrCreateStream(const string& filePath, const int mode) {
		if (_fileMap.find(filePath) == _fileMap.end() || _fileModeMap[filePath] != mode)
			addStream(filePath, mode);

		return *_fileMap.at(filePath);
	}

	void addStream(const string& filePath, const int mode) {
		_fileMap[filePath] = make_unique<ofstream>(filePath, mode);
		_fileModeMap[filePath] = mode;
	}
};
