#pragma once

#include <codecvt>
#include <windows.h>
using namespace std;

class WinApiHelper {
public:
    static constexpr DWORD WAIT_MS_DEFAULT = 5000;
    static constexpr size_t BUFFER_SIZE_DEFAULT = 32 * 1024;

    static bool isValidHandleValue(const HANDLE& handle) {
        return handle != nullptr && handle != INVALID_HANDLE_VALUE;
    }

    static bool isErr(DWORD errCode) {
        return errCode != NO_ERROR;
    }

    static bool waitStatusExited(DWORD waitStatus) {
        return waitStatus == WAIT_OBJECT_0;
    }

    static string replace(const string& input, const string& target, const string& replacement) {
        string result = input;
        size_t startPos = 0;

        while ((startPos = result.find(target, startPos)) != string::npos) {
            result.replace(startPos, target.length(), replacement);
            startPos += replacement.length();
        }

        return result;
    }

    static string join(const string& delim, const vector<string>& strList) {
        string joinedStr;

        for (const auto& str : strList) {
            joinedStr += str + delim;
        }

        if (!joinedStr.empty()) joinedStr = joinedStr.erase(joinedStr.length() - delim.length());
        return joinedStr;
    }

    static wstring convertToW(const string& str) {
        return wstring_convert<codecvt_utf8_utf16<wchar_t>>().from_bytes(str);
    }

    static string convertFromW(const wstring& str) {
        return wstring_convert<codecvt_utf8_utf16<wchar_t>>().to_bytes(str);
    }
};
