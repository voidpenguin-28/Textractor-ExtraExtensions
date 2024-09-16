
#pragma once
#include "../_Libraries/Locker.h"
#include "../_Libraries/strhelper.h"
#include "SharedMemoryInstance.h"
#include <memory>
#include <unordered_map>


class SharedMemoryManager {
public:
	virtual ~SharedMemoryManager() { }
	virtual bool instanceExists(const wstring& identifier) = 0;
	virtual shared_ptr<SharedMemoryInstance> createOrOpenInstance(const wstring& identifier) = 0;
};


class WinApiNamedSharedMemoryManager : public SharedMemoryManager {
public:
	bool instanceExists(const wstring& identifier) override {
		return _lockerMap.getOrCreateLocker(identifier).lockB([this, &identifier]() {
			return instanceExists_(identifier);
		});
	}

	shared_ptr<SharedMemoryInstance> createOrOpenInstance(const wstring& identifier) override {
		shared_ptr<SharedMemoryInstance> instance;
		
		_lockerMap.getOrCreateLocker(identifier).lock([this, &identifier, &instance]() {
			instance = createOrOpenInstance_(identifier);
		});

		return instance;
	}
private:
	static constexpr int BUF_SIZE = 1024;
	const wstring _namePrefix = L"Local\\";
	DefaultLockerMap<wstring> _lockerMap;

	bool instanceExists_(const wstring& identifier) {
		wstring mapName = createMapName(identifier);
		HANDLE hMapFile = openNamedMemMapping(mapName);
		if (!isValidHandle(hMapFile)) return false;

		CloseHandle(hMapFile);
		return true;
	}

	shared_ptr<SharedMemoryInstance> createOrOpenInstance_(const wstring& identifier) {
		wstring mapName = createMapName(identifier);
		HANDLE hMapFile = createOrOpenNamedMemMapping(mapName);
		LPVOID mapView = createNamedMemMapView(hMapFile, mapName);

		shared_ptr<SharedMemoryInstance> instance =
			make_shared<WinApiNamedSharedMemoryInstance>(hMapFile, mapView);

		return instance;
	}

	wstring createMapName(const wstring& identifier) {
		return _namePrefix + identifier;
	}

	bool isValidHandle(const HANDLE handle) {
		return handle != INVALID_HANDLE_VALUE && handle != nullptr;
	}

	HANDLE createOrOpenNamedMemMapping(const wstring& mapName) {
		HANDLE hMapFile = openNamedMemMapping(mapName);
		if (!isValidHandle(hMapFile)) hMapFile = createNamedMemMapping(mapName);
		return hMapFile;
	}

	HANDLE openNamedMemMapping(const wstring& mapName) {
		return OpenFileMapping(
			FILE_MAP_ALL_ACCESS,   // read/write access
			FALSE,                 // do not inherit the name
			mapName.c_str()        // name of mapping object
		);
	}

	HANDLE createNamedMemMapping(const wstring& mapName) {
		HANDLE hMapFile = CreateFileMapping(
			INVALID_HANDLE_VALUE,    // Use paging file
			NULL,                    // Default security
			PAGE_READWRITE,          // Read/write access
			0,                       // Maximum object size (high-order DWORD)
			BUF_SIZE,                // Maximum object size (low-order DWORD)
			mapName.c_str()          // Name of the mapping object
		);

		if (!isValidHandle(hMapFile)) {
			string errMsg = "'CreateFileMapping' failed to create named shared memory space for name: "
				+ StrHelper::convertFromW(mapName) + "; ErrCode: " + to_string(GetLastError());

			throw runtime_error(errMsg);
		}

		return hMapFile;
	}

	LPVOID createNamedMemMapView(const HANDLE hMapFile, const wstring& mapName) {
		LPVOID mapView = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, BUF_SIZE);

		if (mapView == NULL) {
			string errMsg = "'MapViewOfFile' failed to map view of named shared memory space for name: "
				+ StrHelper::convertFromW(mapName) + "; ErrCode: " + to_string(GetLastError());

			if (isValidHandle(hMapFile)) CloseHandle(hMapFile);
			throw runtime_error(errMsg);
		}

		return mapView;
	}
};
