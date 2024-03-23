#pragma once
#include <string>
#include <Windows.h>
using namespace std;

inline void showTextboxMsg(const string& message, const string& appName) {
	MessageBoxA(nullptr, message.c_str(), appName.c_str(), MB_OK);
}

inline void showErrorMessage(const string& message, const string& appName) {
	string tag = appName + "-Error";
	MessageBoxA(nullptr, message.c_str(), tag.c_str(), MB_ICONERROR | MB_OK);
}
