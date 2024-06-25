
#pragma once
#include <windows.h>
#include <psapi.h>
#include <iostream>
#include <string>
using namespace std;

class ProcessNameRetriever {
public:
	virtual ~ProcessNameRetriever() { }
	virtual wstring getProcessName(DWORD pid) const = 0;
};

class WinApiProcessNameRetriever : public ProcessNameRetriever {
public:
	wstring getProcessName(DWORD pid) const override {
		HANDLE hProcess = GetProcessHandle(pid);

		if (isInvalidHandle(hProcess)) {
			cerr << "Error opening process. Error code: " << GetLastError() << endl;
			return L"";
		}

		wstring processNameW = getProcessName(hProcess);
		CloseHandle(hProcess);
		return processNameW;
	}
private:
	static constexpr wchar_t PERIOD_CH = L'.';

	HANDLE GetProcessHandle(DWORD pid) const {
		return OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	}

	bool isInvalidHandle(HANDLE handle) const {
		return handle == nullptr || handle == INVALID_HANDLE_VALUE;
	}

	wstring getProcessName(HANDLE handle) const {
		TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

		if (!GetModuleBaseName(handle, nullptr, szProcessName, sizeof(szProcessName) / sizeof(TCHAR))) {
			cerr << "Error getting process name. Error code: " << GetLastError() << endl;
			return L"";
		}

		wstring processName = wstring(szProcessName);
		return formatProcessName(processName);
	}

	wstring formatProcessName(wstring processName) const {
		size_t periodIndex = processName.rfind(PERIOD_CH);
		if (periodIndex != wstring::npos) {
			processName = processName.substr(0, periodIndex);
		}

		return processName;
	}
};
