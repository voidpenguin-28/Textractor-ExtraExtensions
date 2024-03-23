#pragma once

#include "WinApiHelper.h"
#include <iostream>
#include <string>
#include <windows.h>

class ProcessStateTracker {
public:
	static constexpr DWORD WAIT_MS_DEFAULT = WinApiHelper::WAIT_MS_DEFAULT;
	virtual ~ProcessStateTracker() { }

	virtual bool isProcessActive(const HANDLE& hProcess, DWORD waitForExitMs = 0) const = 0;
	virtual bool waitForProcessExit(const HANDLE& hProcess, DWORD& waitStatus, DWORD& errCode, DWORD waitMs = WAIT_MS_DEFAULT) const = 0;
	virtual bool exitedSuccessfully(const HANDLE& hProcess, DWORD& exitStatus, DWORD& errCode) const = 0;
};


class WinApiProcessStateTracker : public ProcessStateTracker {
public:
	bool isProcessActive(const HANDLE& hProcess, DWORD waitForExitMs = 0) const override {
		DWORD waitStatus, errCode;
		waitForProcessExit(hProcess, waitStatus, errCode, waitForExitMs);

		return !WinApiHelper::isErr(errCode) && !WinApiHelper::waitStatusExited(waitStatus);
	}

	bool waitForProcessExit(const HANDLE& hProcess, DWORD& waitStatus, 
		DWORD& errCode, DWORD waitMs = WAIT_MS_DEFAULT) const override 
	{
		waitStatus = WAIT_OBJECT_0;
		errCode = NO_ERROR;

		if (!WinApiHelper::isValidHandleValue(hProcess)) return true;
		waitStatus = WaitForSingleObject(hProcess, waitMs);

		if (waitStatus == WAIT_FAILED) {
			errCode = GetLastError();
			return false;
		}

		return WinApiHelper::waitStatusExited(waitStatus);
	}

	bool exitedSuccessfully(const HANDLE& hProcess, DWORD& exitStatus, DWORD& errCode) const {
		if (!GetExitCodeProcess(hProcess, &exitStatus)) {
			errCode = GetLastError();
			return false;
		}

		return exitStatus == EXIT_SUCCESS;
	}
};
