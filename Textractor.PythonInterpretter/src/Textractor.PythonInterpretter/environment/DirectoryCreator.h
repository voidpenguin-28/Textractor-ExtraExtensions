#pragma once

#include "../Libraries/strhelper.h"
#include <windows.h>
#include <string>
#include <iostream>
using namespace std;


class DirectoryCreator {
public:
	virtual ~DirectoryCreator() { }
	virtual bool createDir(const wstring& dirPath) const = 0;
	virtual bool createDir(const string& dirPath) const = 0;
protected:
	static constexpr char SEPARATORS[] = { '\\', '/' };
	static constexpr char NO_SEPARATOR = '\0';
};


class NoDirectoryCreator : public DirectoryCreator {
public:
	NoDirectoryCreator(bool returnVal = true) : _returnVal(returnVal) { }

	bool createDir(const wstring& dirPath) const {
		return _returnVal;
	}

	bool createDir(const string& dirPath) const {
		return _returnVal;
	}
private:
	bool _returnVal;
};


class WinApiDirectoryCreator : public DirectoryCreator {
public:
	bool createDir(const wstring& dirPath) const override {
		return createDir(StrHelper::convertFromW(dirPath));
	}

	bool createDir(const string& dirPath) const override {
		char separator = determineSeparator(dirPath);

		string parsedDirPath = containsFileExtension(dirPath) ?
			parseDirectoryFromFilePath(dirPath) : dirPath;

		return runMkDirCommand(parsedDirPath);
	}
private:
	bool containsFileExtension(const string& path, char separator = NO_SEPARATOR) const {
		size_t periodIndex = path.rfind(L'.');
		if (periodIndex == string::npos) return false;

		if (separator == NO_SEPARATOR) separator = determineSeparator(path);
		if (separator == NO_SEPARATOR) return true;

		size_t separatorIndex = path.rfind(path);
		return periodIndex > separatorIndex;
	}

	string parseDirectoryFromFilePath(const string& path, char separator = NO_SEPARATOR) const {
		if (separator == NO_SEPARATOR) separator = determineSeparator(path);
		if (separator == NO_SEPARATOR) return path;

		size_t separatorIndex = path.rfind(separator);
		string dirPath = path.substr(0, separatorIndex);
		return !dirPath.empty() ? dirPath : path;
	}

	char determineSeparator(const string& dirPath) const {
		for (char sep : SEPARATORS) {
			if (dirPath.find(sep) != string::npos) return sep;
		}

		return NO_SEPARATOR;
	}

	bool runMkDirCommand(const string& dirPath) const {
		string command = "cmd /C mkdir \"" + dirPath + "\" > nul 2>&1";
		return runProcess(command);
	}

	bool runProcess(const string& command) const {
		STARTUPINFOW si;
		PROCESS_INFORMATION pi;

		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		si.wShowWindow = SW_HIDE;
		ZeroMemory(&pi, sizeof(pi));
		wstring commandW = StrHelper::convertToW(command);

		if (!CreateProcessW(NULL, (wchar_t*)(commandW.c_str()), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
		{
			showErrorMessage("Failed to run command '" + command
				+ "'. ErrCode: " + to_string(GetLastError()), "TextLogger");

			return false;
		}

		WaitForSingleObject(pi.hProcess, 3000);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return true;
	}
};
