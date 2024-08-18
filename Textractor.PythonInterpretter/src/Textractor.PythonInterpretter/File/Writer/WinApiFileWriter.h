
#pragma once
#include "../../Libraries/Locker.h"
#include "../../Libraries/strhelper.h"
#include "FileWriter.h"
#include <unordered_map>
#include <windows.h>


class WinApiFileWriterBase : public FileWriter {
public:
	virtual ~WinApiFileWriterBase() { }

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

	void writeToFile(const HANDLE hFile, const string& filePath, const string& text) {
		writeToFile(hFile, (text + '\n').c_str());
	}

	void writeToFile(const HANDLE hFile, const string& filePath, 
		const vector<string>& lines, size_t startIndex = 0)
	{
		string text = "";
		for (size_t i = startIndex; i < lines.size(); i++) text += lines[i] + '\n';
		if (!text.empty()) text.erase(text.length() - 1);

		writeToFile(hFile, filePath, text);
	}

	HANDLE createFileHandle(const string& filePath, bool appendMode) {
		DWORD creationFlag = appendMode ? OPEN_ALWAYS : CREATE_ALWAYS;

		HANDLE hFile = CreateFile(
			StrHelper::convertToW(filePath).c_str(),  // name of the write
			FILE_APPEND_DATA,						  // open for appending or writing
			FILE_SHARE_READ | FILE_SHARE_WRITE,		  // share for reading/writing
			NULL,									  // default security
			creationFlag,							  // open existing file or create new file 
			FILE_ATTRIBUTE_NORMAL,					  // normal file
			NULL									  // no attr. template
		);

		if (hFile == INVALID_HANDLE_VALUE) {
			throw runtime_error("Unable to create/open file \"" + filePath + "\" for writing. ErrCode: " + to_string(GetLastError()));
		}

		return hFile;
	}
	
	void writeToFile(const HANDLE hFile, const char* data) {
		DWORD dwBytesToWrite = strlen(data);
		DWORD dwBytesWritten;
		BOOL bErrorFlag = FALSE;

		while (dwBytesToWrite > 0) {
			bErrorFlag = WriteFile(
				hFile,              // open file handle
				data,               // start of data to write
				dwBytesToWrite,     // number of bytes to write
				&dwBytesWritten,    // number of bytes that were written
				NULL                // no overlapped structure
			);

			if (!bErrorFlag)
				throw runtime_error("Failed to write to file. ErrCode: " + to_string(GetLastError()) + "\nText: " + data);

			data += dwBytesWritten;
			dwBytesToWrite -= dwBytesWritten;
		}
	}
};


class WinApiFileWriter : public WinApiFileWriterBase {
protected:
	void writeToFileBase(const string& filePath, const string& text, bool append) override {
		HANDLE f = createFileHandle(filePath, append);
		WinApiFileWriterBase::writeToFile(f, filePath, text);
	}

	void writeToFileBase(const string& filePath, const vector<string>& lines, 
		bool append, size_t startIndex = 0) override 
	{
		HANDLE f = createFileHandle(filePath, append);
		WinApiFileWriterBase::writeToFile(f, filePath, lines, startIndex);
	}
};


// once a file is opened, it stays open unless 'closeAll' or destructor is called
class PersistentWinApiFileWriter : public WinApiFileWriterBase {
public:
	virtual ~PersistentWinApiFileWriter() {
		closeAll();
	}

	void closeAll() {
		for (const auto& f : _fileMap) CloseHandle(f.second);
		_fileMap.clear();
	}
protected:
	void writeToFileBase(const string& filePath, const string& text, bool append) override {
		HANDLE f = getOrCreateStream(filePath, append);
		WinApiFileWriterBase::writeToFile(f, filePath, text);
	}

	void writeToFileBase(const string& filePath, const vector<string>& lines, 
		bool append, size_t startIndex = 0) override
	{
		HANDLE f = getOrCreateStream(filePath, append);
		WinApiFileWriterBase::writeToFile(f, filePath, lines, startIndex);
	}
private:
	unordered_map<string, HANDLE> _fileMap;
	unordered_map<string, bool> _fileModeMap;

	HANDLE getOrCreateStream(const string& filePath, const bool appendMode) {
		if (_fileMap.find(filePath) == _fileMap.end())
			addStream(filePath, appendMode);

		if (_fileModeMap[filePath] != appendMode) {
			CloseHandle(_fileMap[filePath]);
			addStream(filePath, appendMode);
		}

		return _fileMap.at(filePath);
	}

	void addStream(const string& filePath, const bool appendMode) {
		_fileMap[filePath] = createFileHandle(filePath, appendMode);
		_fileModeMap[filePath] = appendMode;
	}
};
