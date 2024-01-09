#pragma once

#include "Libraries/stringconvert.h"
#include "DirectoryCreator.h"
#include <iostream>
#include <fstream>
#include <mutex>
#include <string>
#include <unordered_map>
using namespace std;

class FileWriter {
public:
	virtual ~FileWriter() { }
	virtual void write(const wstring& msg) = 0;
	virtual void write(const string& msg) = 0;
};

class OFStreamFileWriter : public FileWriter {
public:
	OFStreamFileWriter(const DirectoryCreator& dirCreator, const wstring& filePath)
		: _dirCreator(dirCreator), _filePath(filePath) 
	{
		if (!_dirCreator.createDir(_filePath))
			throw runtime_error("Failed to create directory for file path: " + convertFromW(_filePath));
		
		_fStream = ofstream(_filePath, FILE_MODE);

		if (!_fStream.is_open())
			throw runtime_error("Failed to open file: " + convertFromW(_filePath));
	}

	~OFStreamFileWriter() override {
		_fStream.close();
	}

	void write(const wstring& msg) override {
		write(convertFromW(msg));
	}

	void write(const string& msg) override {
		lock_guard<mutex> lock(_mutex);
		writeBase(msg);
	}
private:
	static constexpr ios_base::openmode FILE_MODE = std::ios::out | std::ios::app;
	const DirectoryCreator& _dirCreator;
	const wstring _filePath;
	ofstream _fStream;
	mutex _mutex;

	void writeBase(const string& msg) {
		_fStream << msg << endl;
	}
};


class FileWriterFactory {
public:
	virtual ~FileWriterFactory() { }
	virtual FileWriter* createFileWriter(const wstring& filePath) const = 0;
};

class OFStreamFileWriterFactory : public FileWriterFactory {
public:
	OFStreamFileWriterFactory(const DirectoryCreator& dirCreator) : _dirCreator(dirCreator) { }

	FileWriter* createFileWriter(const wstring& filePath) const override {
		return new OFStreamFileWriter(_dirCreator, filePath);
	}
private:
	const DirectoryCreator& _dirCreator;
};

class FileWriterManager {
public:
	virtual ~FileWriterManager() { }
	virtual void write(const wstring& filePath, const wstring& msg) = 0;
	virtual void write(const wstring& filePath, const string& msg) = 0;
};

class FactoryFileWriterManager : public FileWriterManager {
public:
	FactoryFileWriterManager(FileWriterFactory& writerFactory) : _writerFactory(writerFactory) { }
	
	~FactoryFileWriterManager() override {
		disposeAllWriters();
	}

	void write(const wstring& filePath, const wstring& msg) override {
		lock_guard<mutex> lock(_mutex);
		write_(filePath, msg);
	}

	void write(const wstring& filePath, const string& msg) override {
		lock_guard<mutex> lock(_mutex);
		write_(filePath, msg);
	}
private:
	FileWriterFactory& _writerFactory;
	unordered_map<wstring, FileWriter*> _filePathToWriterMap = { };
	mutex _mutex;

	void write_(const wstring& filePath, const wstring& msg) {
		FileWriter* writer = getOrCreateWriter(filePath);
		writer->write(msg);
	}

	void write_(const wstring& filePath, const string& msg) {
		FileWriter* writer = getOrCreateWriter(filePath);
		writer->write(msg);
	}

	FileWriter* getOrCreateWriter(const wstring& filePath) {
		if (!hasWriter(filePath)) setWriter(filePath);
		return _filePathToWriterMap[filePath];
	}

	bool hasWriter(const wstring& filePath) const {
		return _filePathToWriterMap.find(filePath) != _filePathToWriterMap.end();
	}

	void setWriter(const wstring& filePath) {
		_filePathToWriterMap[filePath] = _writerFactory.createFileWriter(filePath);
	}


	void disposeAllWriters() {
		for (auto& pair : _filePathToWriterMap) {
			delete pair.second;
		}

		_filePathToWriterMap.clear();
	}
};
