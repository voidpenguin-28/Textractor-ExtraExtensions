
#pragma once
#include "../_Libraries/Locker.h"
#include <string>
#include <windows.h>
using namespace std;


class SharedMemoryInstance {
public:
	virtual ~SharedMemoryInstance() { }
	virtual void writeData(const wstring& str) = 0;
	virtual wstring readData() const = 0;
	virtual void close() = 0;
};


class WinApiNamedSharedMemoryInstance : public SharedMemoryInstance {
public:
	WinApiNamedSharedMemoryInstance(HANDLE hMapFile, LPVOID mapView) : _hMapFile(hMapFile), _mapView(mapView) { }

	virtual ~WinApiNamedSharedMemoryInstance() {
		close_();
	}

	void writeData(const wstring& str) override {
		_locker.lock([this, &str]() {
			writeData_(str);
		});
	}

	wstring readData() const override {
		return _locker.lockWS([this]() {
			return readData_();
		});
	}

	void close() override {
		_locker.lock([this]() {
			close_();
		});
	}
private:
	mutable BasicLocker _locker;
	HANDLE _hMapFile;
	LPVOID _mapView;

	void writeData_(const wstring& str) {
		lstrcpyW((wchar_t*)_mapView, str.c_str());
	}

	wstring readData_() const {
		wchar_t* cwStr = static_cast<wchar_t*>(_mapView);
		return wstring(cwStr);
	}

	void close_() {
		unmapView();
		closeMapHandle();
	}

	bool isValidHandle(const HANDLE handle) {
		return handle != INVALID_HANDLE_VALUE && handle != nullptr;
	}

	void closeMapHandle() {
		if (!isValidHandle(_hMapFile)) return;
		CloseHandle(_hMapFile);
		_hMapFile = nullptr;
	}

	void unmapView() {
		if (_mapView == NULL) return;
		UnmapViewOfFile(_mapView);
		_mapView = NULL;
	}
};
