#pragma once

#include "../Libraries/Locker.h"
#include "../Libraries/strhelper.h"
#include "../logging/LoggerBase.h"
#include "WinApiHelper.h"
#include <iostream>
#include <string>
#include <windows.h>
using namespace std;


class PipeManager {
public:
	PipeManager(const string& pipeName, const size_t bufferSize = WinApiHelper::BUFFER_SIZE_DEFAULT) 
		: PipeName(pipeName), BufferSize(bufferSize) { }
	
	virtual ~PipeManager() { }

	const string PipeName;
	const size_t BufferSize;

	virtual bool isPipeEnabled() const = 0;
	virtual bool isPipeActive(DWORD waitForExitMs = 0) const = 0;
	virtual DWORD createPipe(bool connectPipe) = 0;
	virtual DWORD connectToPipe() = 0;

	virtual DWORD closePipe() = 0;
	virtual DWORD disconnectFromPipe() = 0;

	virtual DWORD writeToPipe(const string& command) = 0;
	virtual DWORD readFromPipe(string& output) = 0;
};


class ThreadSafePipeManager : public PipeManager {
public:
	ThreadSafePipeManager(unique_ptr<PipeManager> rootPipeManager)
		: PipeManager(rootPipeManager->PipeName, rootPipeManager->BufferSize), 
			_rootPipeManager(move(rootPipeManager)) { }

	virtual ~ThreadSafePipeManager() { }

	bool isPipeEnabled() const override {
		_coreLocker.waitForUnlock();
		return _semaLocker.lockB(_isPipeEnabled);
	}

	bool isPipeActive(DWORD waitForExitMs = 0) const override {
		_coreLocker.waitForUnlock();

		return _semaLocker.lockB([this, waitForExitMs]() {
			return _rootPipeManager->isPipeActive(waitForExitMs);
		});
	}

	DWORD createPipe(bool connectPipe) override {
		return _coreLocker.lockDW([this, connectPipe]() {
			_semaLocker.waitForAllUnlocked();
			return _rootPipeManager->createPipe(connectPipe);
		});
	}

	DWORD connectToPipe() override {
		return _coreLocker.lockDW(_connectToPipe);
	}

	DWORD closePipe() override {
		return _coreLocker.lockDW(_closePipe);
	}

	DWORD disconnectFromPipe() override {
		return _coreLocker.lockDW(_disconnectFromPipe);
	}

	DWORD writeToPipe(const string& command) override {
		return _coreLocker.lockDW([this, &command]() {
			_semaLocker.waitForAllUnlocked();
			return _rootPipeManager->writeToPipe(command);
		});
	}

	DWORD readFromPipe(string& output) override {
		return _coreLocker.lockDW([this, &output]() {
			_semaLocker.waitForAllUnlocked();
			return _rootPipeManager->readFromPipe(output);
		});
	}
private:
	unique_ptr<PipeManager> _rootPipeManager;
	mutable BasicLocker _coreLocker;
	mutable SemaphoreLocker _semaLocker;
	
	const function<DWORD()> _isPipeEnabled = [this]() { return _rootPipeManager->isPipeEnabled(); };
	const function<DWORD()> _connectToPipe = [this]() {
		_semaLocker.waitForAllUnlocked();
		return _rootPipeManager->connectToPipe();
	};
	const function<DWORD()> _closePipe = [this]() {
		_semaLocker.waitForAllUnlocked();
		return _rootPipeManager->closePipe();
	};
	const function<DWORD()> _disconnectFromPipe = [this]() {
		_semaLocker.waitForAllUnlocked();
		return _rootPipeManager->disconnectFromPipe();
	};
};


// this class is not fully thread-safe; use with ThreadSafeDecoratorPipeManager to make thread-safe
class WinApiPipeManager : public PipeManager {
public:
	WinApiPipeManager(const Logger& logger, const string& pipeName, 
		const size_t bufferSize = WinApiHelper::BUFFER_SIZE_DEFAULT)
		: PipeManager(pipeName, bufferSize), _logger(logger), _pipePath(createPipePath(pipeName)), 
			_pipePathC(_pipePath.c_str()), _buffer(new char[BufferSize]) { }

	virtual ~WinApiPipeManager() {
		if (isPipeEnabled()) closePipe();
		delete[] _buffer;
	}

	bool isPipeEnabled() const override {
		return WinApiHelper::isValidHandleValue(_pipe);
	}

	bool isPipeActive(DWORD waitForExitMs = 0) const override {
		return waitForExitMs > 0 ?
			isPipeActiveViaWaitNamedPipe(waitForExitMs) : 
			isPipeActiveViaCreateFile();
	}

	DWORD createPipe(bool connectPipe) override {
		_logger.logInfo("Creating pipe: " + PipeName);

		if (isPipeEnabled()) {
			_logger.logWarning("Attempted to create pipe that already exists. Reusing pipe: " + PipeName);
			return NO_ERROR;
		}

		_pipe = CreateNamedPipe(
			_pipePathC, // pipe name
			PIPE_ACCESS_DUPLEX, // pipe open mode
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, // pipe mode
			1, // max instances
			BufferSize, // output buffer size
			BufferSize, // input buffer size
			0, // default timeout
			NULL //security attributes
		);

		if (!isPipeEnabled()) {
			DWORD errCode = GetLastError();
			_logger.logError("Failed to create pipe '" +
				StrHelper::convertFromW(_pipePath) + "'. ErrCode: " + to_string(errCode));

			_pipe = nullptr;
			return errCode;
		}

		if (connectPipe) {
			DWORD errCode = connectToPipe();

			if (WinApiHelper::isErr(errCode)) {
				closePipe();
				return errCode;
			}
		}

		return NO_ERROR;
	}

	DWORD connectToPipe() override {
		_logger.logInfo("Connecting to pipe: " + PipeName);

		if (!ConnectNamedPipe(_pipe, NULL)) {
			DWORD errCode = GetLastError();
			
			if (errCode != ERROR_PIPE_CONNECTED) {
				_logger.logError("Failed to connect to pipe '" +
					PipeName + "'. ErrCode: " + to_string(errCode));

				return errCode;
			}
		}

		return NO_ERROR;
	}

	DWORD closePipe() override {
		_logger.logInfo("Closing pipe handle: " + PipeName);

		if (!isPipeEnabled()) {
			_logger.logWarning("Attempted to close non-existant pipe handle: " + PipeName);
		}

		DWORD disconnectErrCode = disconnectFromPipe();
		DWORD closeErrorCode = _closePipeHandle();

		if (WinApiHelper::isErr(closeErrorCode)) return closeErrorCode;
		return disconnectErrCode;
	}

	DWORD disconnectFromPipe() override {
		_logger.logInfo("Disconnecting from pipe: " + PipeName);

		if (!isPipeEnabled()) {
			_logger.logWarning("Attempted to disconnect from non-existant pipe handle: " + PipeName);
			return NO_ERROR;
		}

		if (!DisconnectNamedPipe(_pipe)) {
			DWORD errCode = GetLastError();

			if (errCode != ERROR_PIPE_NOT_CONNECTED) {
				_logger.logError("Failed to disconnect to pipe '" +
					PipeName + "'. ErrCode: " + to_string(errCode));

				return errCode;
			}
		}

		return NO_ERROR;
	}

	DWORD writeToPipe(const string& command) override {
		_logger.logDebug("Writing data to pipe '" + PipeName + "': " 
			+ (command.length() < 300 ? command : command.substr(0, 100) + "...\n"));

		DWORD bytesRead = 0;
		const char* commandC = command.c_str();

		if (!WriteFile(_pipe, commandC, strlen(commandC), &bytesRead, NULL)) {
			DWORD errCode = GetLastError();
			_logger.logError("Failed to write to pipe '" + 
				PipeName + "'. ErrCode: " + to_string(errCode));

			return errCode;
		}

		if (!FlushFileBuffers(_pipe)) {
			DWORD errCode = GetLastError();
			_logger.logError("Failed to flush buffer while writing to pipe '"
				+ PipeName + "'. ErrCode: " + to_string(errCode));
			
			return errCode;
		}

		return NO_ERROR;
	}

	DWORD readFromPipe(string& output) override {
		DWORD bytesRead = 0, bytesAvail = 0;
		output = "";

		do {
			if (!ReadFile(_pipe, _buffer, BufferSize, &bytesRead, NULL)) {
				DWORD errCode = GetLastError();

				if (errCode != ERROR_MORE_DATA) {
					_logger.logError("Failed to read from pipe '"
						+ PipeName + "'. ErrCode: " + to_string(errCode));

					return errCode;
				}
			}

			if (bytesRead < BufferSize) _buffer[bytesRead] = '\0';
			output.append(_buffer, bytesRead);
		} while (PeekNamedPipe(_pipe, NULL, 0, NULL, &bytesAvail, NULL) && bytesAvail > 0);

		_logger.logDebug("Data read from pipe '" + PipeName + "': " + output);
		return NO_ERROR;
	}
private:
	const Logger& _logger;
	const wstring _pipePath;
	const wchar_t* _pipePathC;
	HANDLE _pipe = nullptr;
	char* _buffer;

	static wstring createPipePath(const string& pipeName) {
		return L"\\\\.\\pipe\\" + StrHelper::convertToW(pipeName);
	}

	bool isPipeActiveViaWaitNamedPipe(DWORD waitForExitMs = 0) const {
		if (!WaitNamedPipe(_pipePathC, waitForExitMs)) {
			DWORD errCode = GetLastError();
			if (errCode == ERROR_SEM_TIMEOUT) return true;
		}
		
		return false;
	}

	bool isPipeActiveViaCreateFile() const {
		HANDLE handle = CreateFile(_pipePathC, GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL
		);

		if (handle == INVALID_HANDLE_VALUE)
		{
			DWORD errCode = GetLastError();
			if (errCode == ERROR_PIPE_BUSY) return true;
		}
		else CloseHandle(handle);

		return false;
	}

	DWORD _closePipeHandle() {
		DWORD errCode = NO_ERROR;
		if (!isPipeEnabled()) return errCode;

		if (!CloseHandle(_pipe)) {
			errCode = GetLastError();
			_logger.logError("Failed to close handle for pipe '" 
				+ PipeName + "'. ErrCode: " + to_string(errCode));
		}

		_pipe = nullptr;
		return errCode;
	}
};
