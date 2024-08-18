
#pragma once
#include "strhelper.h"
#include <string>
#include <windows.h>
using namespace std;


class FileTracker {
public:
	virtual ~FileTracker() { }
	virtual int64_t getDateLastModifiedEpochs(const string& filePath) = 0;
	virtual int64_t getDateLastModifiedEpochs(const char* filePath) = 0;
	virtual int64_t getDateLastModifiedEpochs(const wstring& filePath) = 0;
	virtual int64_t getDateLastModifiedEpochs(const wchar_t* filePath) = 0;
};


class WinApiFileTracker : public FileTracker {
public:
	int64_t getDateLastModifiedEpochs(const string& filePath) override {
		return getDateLastModifiedEpochs(StrHelper::convertToW(filePath));
	}

	int64_t getDateLastModifiedEpochs(const char* filePath) override {
		return getDateLastModifiedEpochs(string(filePath));
	}

	int64_t getDateLastModifiedEpochs(const wstring& filePath) override {
		return getDateLastModifiedEpochs(filePath.c_str());
	}

	int64_t getDateLastModifiedEpochs(const wchar_t* filePath) override {
		HANDLE fHandle = getFileHandle(filePath);
		if (!isValidHandle(fHandle)) return 0;

		FILETIME lastWrite = getFileLastWriteTime(fHandle);
		CloseHandle(fHandle);

		return convertTo64(lastWrite);
	}
private:
	HANDLE getFileHandle(const wchar_t* filePath) {
		return CreateFile(filePath, GENERIC_READ, 
			FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	}

	bool isValidHandle(const HANDLE& handle) {
		return handle != INVALID_HANDLE_VALUE;
	}

	FILETIME getFileLastWriteTime(const HANDLE& handle) {
		FILETIME lastWrite;
		GetFileTime(handle, NULL, NULL, &lastWrite);
		return lastWrite;
	}

	int64_t convertTo64(FILETIME fTime) {
		ULARGE_INTEGER ularge{};
		ularge.LowPart = fTime.dwLowDateTime;
		ularge.HighPart = fTime.dwHighDateTime;
		return static_cast<int64_t>(ularge.QuadPart);
	}
};
