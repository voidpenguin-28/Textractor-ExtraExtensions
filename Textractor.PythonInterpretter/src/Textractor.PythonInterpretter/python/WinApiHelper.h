#pragma once

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
};
