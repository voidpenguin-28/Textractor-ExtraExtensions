#pragma once

#include "../Libraries/strhelper.h"
#include "../logging/LoggerBase.h"
#include "ProcessStateTracker.h"
#include "WinApiHelper.h"
#include <future>
#include <iostream>
#include <string>
#include <windows.h>

class WinEventHandler {
public:
	virtual ~WinEventHandler() { }
	static constexpr DWORD WAIT_MS_DEFAULT = WinApiHelper::WAIT_MS_DEFAULT;

	virtual future<pair<bool, DWORD>> createEventAndWaitAsync(const string& eventName, DWORD waitMs = WAIT_MS_DEFAULT) const = 0;
	virtual bool createEventAndWait(const string& eventName, DWORD& errCode, DWORD waitMs = WAIT_MS_DEFAULT) const = 0;
	virtual bool respondToEvent(const string& eventName, DWORD& errCode) const = 0;
};


class WinApiWinEventHandler : public WinEventHandler {
public:
	WinApiWinEventHandler(const Logger& logger, const ProcessStateTracker& stateTracker) 
		: _logger(logger), _stateTracker(stateTracker) { }


	future<pair<bool, DWORD>> createEventAndWaitAsync(
		const string& eventName, DWORD waitMs = WAIT_MS_DEFAULT) const override
	{
		DWORD errCode;
		HANDLE hEvent = createEvent(eventName, errCode);
		
		auto asyncFunc = [this, eventName, hEvent, errCode, waitMs]() {
			DWORD finalErrCode = errCode;
			bool exited = waitForEventAndClose(eventName, hEvent, finalErrCode, waitMs);
			return pair<bool, DWORD>(exited, finalErrCode);
		};

		return async(launch::async, asyncFunc);
	}

	bool createEventAndWait(const string& eventName, 
		DWORD& errCode, DWORD waitMs = WAIT_MS_DEFAULT) const override
	{
		HANDLE hEvent = createEvent(eventName, errCode);
		return waitForEventAndClose(eventName, hEvent, errCode, waitMs);
	}

	bool respondToEvent(const string& eventName, DWORD& errCode) const override {
		HANDLE hEvent = openEvent(eventName, errCode);
		if (!isValidEvent(hEvent, errCode)) return false;

		bool eventSet = setEvent(eventName, hEvent, errCode);

		DWORD closeErrCode = closeHandle(eventName, hEvent);
		if (!WinApiHelper::isErr(errCode)) errCode = closeErrCode;
		
		return eventSet;
	}
private:
	const Logger& _logger;
	const ProcessStateTracker& _stateTracker;

	bool isValidEvent(const HANDLE& hEvent, const DWORD& errCode) const {
		if (!WinApiHelper::isValidHandleValue(hEvent)) return false;
		if (WinApiHelper::isErr(errCode)) return false;

		return true;
	}

	HANDLE createEvent(const string& eventName, DWORD& errCode) const {
		_logger.logInfo("Creating win event: " + eventName);
		errCode = NO_ERROR;
		wstring eventNameW = StrHelper::convertToW(eventName);
		HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, eventNameW.c_str());

		if (!WinApiHelper::isValidHandleValue(hEvent)) {
			errCode = GetLastError();
			_logger.logError("Failed to create event object. EventName: "
				+ eventName + "; ErrCode: " + to_string(errCode));

			hEvent = nullptr;
		}

		return hEvent;
	}

	bool waitForEventAndClose(const string& eventName, const HANDLE& hEvent,
		DWORD& errCode, DWORD waitMs = WAIT_MS_DEFAULT) const
	{
		if (!isValidEvent(hEvent, errCode)) return false;

		bool exited = waitForEventResponse(eventName, hEvent, errCode, waitMs);

		DWORD closeErrCode = closeHandle(eventName, hEvent);
		if (!WinApiHelper::isErr(errCode)) errCode = closeErrCode;

		return exited;
	}

	bool waitForEventResponse(const string& eventName, 
		const HANDLE& hEvent, DWORD& errCode, DWORD waitMs = WAIT_MS_DEFAULT) const
	{
		_logger.logInfo("Waiting for win event '" + eventName
			+ "' response from client for " + to_string(waitMs) + "ms");

		DWORD waitStatus;

		if (!_stateTracker.waitForProcessExit(hEvent, waitStatus, errCode, waitMs)) {
			_logger.logError("Failed to receive event signal from client. EventName: "
				+ eventName + "; WaitStatus: " + to_string(waitStatus)
				+ "; ErrCode: " + to_string(errCode));

			return false;
		}

		_logger.logInfo("Client response received for win event: " + eventName);
		return true;
	}

	HANDLE openEvent(const string& eventName, DWORD& errCode) const {
		_logger.logInfo("Opening win event: " + eventName);
		errCode = NO_ERROR;
		wstring eventNameW = StrHelper::convertToW(eventName);
		HANDLE hEvent = OpenEvent(EVENT_MODIFY_STATE, false, eventNameW.c_str());

		if (!WinApiHelper::isValidHandleValue(hEvent)) {
			errCode = GetLastError();
			_logger.logError("Failed to open event. EventName: " 
				+ eventName + "; ErrCode" + to_string(errCode));

			return nullptr;
		}

		return hEvent;
	}

	bool setEvent(const string& eventName, const HANDLE& hEvent, DWORD& errCode) const {
		_logger.logInfo("Setting win event: " + eventName);
		errCode = NO_ERROR;

		if (!SetEvent(hEvent)) {
			errCode = GetLastError();
			_logger.logError("Failed to set event. EventName: " 
				+ eventName + "; ErrCode: " + to_string(errCode));

			return false;
		}

		return true;
	}

	DWORD closeHandle(const string& eventName, const HANDLE& hEvent) const {
		DWORD errCode = NO_ERROR;
		if (!WinApiHelper::isValidHandleValue(hEvent)) return errCode;

		if (!CloseHandle(hEvent)) {
			errCode = GetLastError();
			_logger.logError("Failed to close event handle. EventName: "
				+ eventName + "; ErrCode: " + to_string(errCode));
		}

		return errCode;
	}
};
