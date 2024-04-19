
#pragma once
#include "stringconvert.h"
#include <string>
#include <windows.h>
using namespace std;

inline void showErrorMessage(const string& message, const string& appName) {
	string tag = appName + "-Error";
	MessageBoxA(nullptr, message.c_str(), tag.c_str(), MB_ICONERROR | MB_OK);
}

inline string getModuleName(const HMODULE& handle) {
	try {
		wchar_t buffer[1024];
		GetModuleFileName(handle, buffer, sizeof(buffer) / sizeof(wchar_t));

		string module = convertFromW(buffer);
		size_t pathDelimIndex = module.rfind('\\');
		if (pathDelimIndex != string::npos) module = module.substr(pathDelimIndex + 1);

		size_t extIndex = module.rfind('.');
		if (extIndex != string::npos) module = module.erase(extIndex);

		return module;
	}
	catch (exception& ex) {
		string errMsg = "Failed to retrieve extension name.\n";
		errMsg += ex.what();
		showErrorMessage(errMsg.c_str(), "TextLogger");
		throw;
	}
}
