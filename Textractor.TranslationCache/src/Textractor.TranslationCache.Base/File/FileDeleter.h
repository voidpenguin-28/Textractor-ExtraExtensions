
#pragma once
#include <string>
using namespace std;


class FileDeleter {
public:
	virtual ~FileDeleter() { }
	virtual void deleteFile(const string& filePath) = 0;
};


class CRemoveFileDeleter : public FileDeleter {
public:
	void deleteFile(const string& filePath) override {
		remove(filePath.c_str());
	}
};
