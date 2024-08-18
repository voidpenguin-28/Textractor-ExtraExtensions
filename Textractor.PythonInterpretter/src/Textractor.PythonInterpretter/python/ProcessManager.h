#pragma once

#include "../Libraries/Locker.h"
#include "../Libraries/strhelper.h"
#include "../logging/LoggerBase.h"
#include "ProcessStateTracker.h"
#include "WinApiHelper.h"
#include "WinEventHandler.h"
#include <iostream>
#include <string>
#include <windows.h>
using namespace std;

class ProcessManager {
public:
	static constexpr DWORD WAIT_MS_DEFAULT = 5000;

	ProcessManager(const string& identifier) : Identifier(identifier) { }
	virtual ~ProcessManager() { }
	
	const string Identifier;

	virtual bool isProcessEnabled() const = 0;
	virtual bool isProcessActive(DWORD waitForExitMs = 0) const = 0;
	
	virtual bool waitForProcessExit(DWORD waitMs = WAIT_MS_DEFAULT) const = 0;
	virtual bool waitForProcessExit(DWORD& waitStatus, DWORD& errCode, DWORD waitMs = WAIT_MS_DEFAULT) const = 0;

	virtual bool exitedSuccessfully() const = 0;
	virtual bool exitedSuccessfully(DWORD& exitStatus, DWORD& errCode) const = 0;

	virtual DWORD createProcess(const string& command, bool hideWindow) = 0;
	virtual DWORD closeProcess() = 0;
};


class ThreadSafeProcessManager : public ProcessManager {
public:
	ThreadSafeProcessManager(ProcessManager& rootProcManager)
		: ProcessManager(rootProcManager.Identifier), _rootProcManager(rootProcManager) { }

	virtual ~ThreadSafeProcessManager() { }

	bool isProcessEnabled() const override {
		_coreLocker.waitForUnlock();
		return _semaLocker.lockB(_isProcessEnabled);
	}

	bool isProcessActive(DWORD waitForExitMs = 0) const override {
		_coreLocker.waitForUnlock();

		return _semaLocker.lockB([this, waitForExitMs]() {
			return _rootProcManager.isProcessActive(waitForExitMs);
		});
	}

	bool waitForProcessExit(DWORD waitMs = WAIT_MS_DEFAULT) const override {
		_coreLocker.waitForUnlock();

		return _semaLocker.lockB([this, waitMs]() {
			return _rootProcManager.waitForProcessExit(waitMs);
		});
	}

	bool waitForProcessExit(DWORD& waitStatus, DWORD& errCode, DWORD waitMs = WAIT_MS_DEFAULT) const override {
		_coreLocker.waitForUnlock();

		return _semaLocker.lockB([this, &waitStatus, &errCode, waitMs]() {
			return _rootProcManager.waitForProcessExit(waitStatus, errCode, waitMs);
		});
	}

	virtual bool exitedSuccessfully() const {
		_coreLocker.waitForUnlock();
		return _semaLocker.lockB(_exitedSuccessfully);
	}

	virtual bool exitedSuccessfully(DWORD& exitStatus, DWORD& errCode) const {
		_coreLocker.waitForUnlock();

		return _semaLocker.lockB([this, &exitStatus, &errCode]() {
			return _rootProcManager.exitedSuccessfully(exitStatus, errCode);
		});
	}

	virtual DWORD createProcess(const string& command, bool hideWindow) override {
		return _coreLocker.lockDW([this, &command, hideWindow]() {
			_semaLocker.waitForAllUnlocked();
			return _rootProcManager.createProcess(command, hideWindow);
		});
	}

	virtual DWORD closeProcess() override {
		return _coreLocker.lockDW(_closeProcess);
	}
private:
	ProcessManager& _rootProcManager;
	mutable BasicLocker _coreLocker;
	mutable SemaphoreLocker _semaLocker;
	
	const function<bool()> _isProcessEnabled = [this]() { return _rootProcManager.isProcessEnabled(); };
	const function<bool()> _exitedSuccessfully = [this]() {
		return _rootProcManager.exitedSuccessfully();
	};
	const function<DWORD()> _closeProcess = [this]() {
		_semaLocker.waitForAllUnlocked();
		return _rootProcManager.closeProcess();
	};
};


// this class is not fully thread-safe; use with ThreadSafeDecoratorProcessManager to make thread-safe
class WinApiProcessManager : public ProcessManager {
public:
	WinApiProcessManager(const string& identifier, const Logger& logger,
		const ProcessStateTracker& stateTracker) 
		: ProcessManager(identifier), _logger(logger), _stateTracker(stateTracker) { }

	virtual ~WinApiProcessManager() {
		if (isProcessEnabled()) closeProcess();
	}

	bool isProcessEnabled() const override {
		return WinApiHelper::isValidHandleValue(_pi.hProcess)
			&& WinApiHelper::isValidHandleValue(_pi.hThread);
	}

	bool isProcessActive(DWORD waitForExitMs = 0) const override {
		HANDLE hProcess = getProcHandle();
		return _stateTracker.isProcessActive(hProcess, waitForExitMs);
	}

	bool waitForProcessExit(DWORD waitMs = WAIT_MS_DEFAULT) const override {
		DWORD waitStatus, errCode;
		return waitForProcessExit(waitStatus, errCode, waitMs);
	}

	bool waitForProcessExit(DWORD& waitStatus, DWORD& errCode, DWORD waitMs = WAIT_MS_DEFAULT) const override {
		HANDLE hProcess = getProcHandle();
		bool exited = _stateTracker.waitForProcessExit(hProcess, waitStatus, errCode, waitMs);

		if (WinApiHelper::isErr(errCode)) {
			_logger.logError("Failed to check if process is active. Identifier: "
				+ Identifier + "; WaitStatus: " + to_string(waitStatus)
				+ "; ErrCode: " + to_string(errCode));
		}

		return exited;
	}

	bool exitedSuccessfully() const override {
		DWORD exitStatus, errCode;
		return exitedSuccessfully(exitStatus, errCode);
	}

	bool exitedSuccessfully(DWORD& exitStatus, DWORD& errCode) const override {
		HANDLE hProcess = getProcHandle();
		bool exited = _stateTracker.exitedSuccessfully(hProcess, exitStatus, errCode);

		if (WinApiHelper::isErr(errCode)) {
			_logger.logError("Failed to check if process has exited successfully. Identifier: "
				+ Identifier + "; ExitStatus: " + to_string(exitStatus)
				+ "; ErrCode: " + to_string(errCode));
		}

		return exited;
	}

	DWORD createProcess(const string& command, bool hideWindow) override {
		_logger.logInfo("Starting process for identifier: " + Identifier);
		_logger.logDebug("Process start command for identifier '" + Identifier + "': " + command);
		
		if (isProcessEnabled()) {
			_logger.logWarning("Attempted to create process that's already in use with this identifier. Reusing process (identifier): " + Identifier);
			return NO_ERROR;
		}

		SECURITY_ATTRIBUTES sa{};
		sa.nLength = sizeof(sa);
		sa.bInheritHandle = TRUE;
		sa.lpSecurityDescriptor = NULL;
		ZeroMemory(&_pi, sizeof(PROCESS_INFORMATION));
		DWORD processFlags = createProcessFlags(hideWindow);

		STARTUPINFO si;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		if(hideWindow) si.wShowWindow = SW_HIDE;

		return createProcess(command, si, processFlags);
	}

	DWORD closeProcess() override {
		_logger.logInfo("Closing process for identifier: " + Identifier);
		DWORD errCode = NO_ERROR;

		if (!isProcessEnabled()) {
			_logger.logWarning("Attempted to close a non-existant process for identifier: " + Identifier);
			return errCode;
		}

		if (isProcessActive()) {
			_logger.logWarning("Process has not yet terminated organically. Forcing process termination for identifier: " + Identifier);
			errCode = terminateProcess();
		}

		DWORD closeErrCode = closeHandles();
		clearProcInfo();

		return WinApiHelper::isErr(errCode) ? errCode : closeErrCode;
	}
private:
	const Logger& _logger;
	const ProcessStateTracker& _stateTracker;
	PROCESS_INFORMATION _pi{};
	
	HANDLE getProcHandle() const {
		return _pi.hProcess;
	}

	DWORD createProcessFlags(bool hideWindow) const {
		DWORD processFlags = CREATE_UNICODE_ENVIRONMENT;
		processFlags |= hideWindow ? CREATE_NO_WINDOW : CREATE_NEW_CONSOLE;
		return processFlags;
	}

	DWORD createProcess(const string& command, STARTUPINFO& si, DWORD processFlags) {
		wstring commandW = StrHelper::convertToW(command);

		if (!CreateProcess(NULL, (wchar_t*)(commandW.c_str()), NULL, NULL, TRUE, processFlags, NULL, NULL, &si, &_pi)) {
			DWORD errCode = GetLastError();
			_logger.logError("Failed to create process. Identifier '" + Identifier
				+ "'. ErrCode: " + to_string(errCode) + "; Command: " + command);

			closeHandles();
			return errCode;
		}

		return NO_ERROR;
	}

	DWORD terminateProcess() const {
		DWORD errCode = TerminateProcess(_pi.hProcess, 0);

		if (WinApiHelper::isErr(errCode)) {
			_logger.logError("Failed to terminate process. Identifier '" 
				+ Identifier + "'; ErrCode: " + to_string(errCode));
		}

		return errCode;
	}

	DWORD closeHandles() const {
		DWORD procErrCode = closeHandle(_pi.hProcess);
		DWORD threadErrCode = closeHandle(_pi.hThread);
		return WinApiHelper::isErr(procErrCode) ? procErrCode : threadErrCode;
	}

	DWORD closeHandle(const HANDLE& handle) const {
		DWORD errCode = NO_ERROR;
		if (!isProcessEnabled()) return errCode;

		if (!CloseHandle(handle)) {
			errCode = GetLastError();
			_logger.logError("Failed to close process handle. Identifier: " 
				+ Identifier + "; ErrCode: " + to_string(errCode));
		}

		return errCode;
	}

	void clearProcInfo() {
		_pi.hProcess = _pi.hThread = nullptr;
		_pi.dwProcessId = _pi.dwThreadId = 0;
	}
};
