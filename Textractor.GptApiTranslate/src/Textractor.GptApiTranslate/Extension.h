
#pragma once

#define WIN32_LEAN_AND_MEAN
#include "_Libraries/strhelper.h"
#include <windows.h>
#include <cstdint>
#include <string>
using namespace std;


struct InfoForExtension
{
	const char* name;
	int64_t value;
};

struct SentenceInfo
{
	const InfoForExtension* infoArray;
	int64_t operator[](std::string propertyName)
	{
		for (auto info = infoArray; info->name; ++info) // nullptr name marks end of info array
			if (propertyName == info->name) return info->value;
		return *(int*)0xcccc = 0; // gives better error message than alternatives
	}
};

struct SKIP {};
inline void Skip() { throw SKIP(); }



class SentenceInfoWrapper {
public:
	SentenceInfoWrapper(SentenceInfo& sentenceInfo) : _sentenceInfo(sentenceInfo) { }

	bool isActiveThread() {
		return getCurrentSelect();
	}

	bool threadIsConsoleOrClipboard() {
		return getProcessId() == 0;
	}

	int64_t getCurrentSelect() {
		return _sentenceInfo["current select"];
	}

	wstring getProcessIdW() {
		return to_wstring(getProcessId());
	}

	DWORD getProcessIdD() {
		return static_cast<DWORD>(getProcessId());
	}

	int64_t getProcessId() {
		return _sentenceInfo["process id"];
	}

	wstring getThreadNumberW() {
		int64_t threadNum = getThreadNumber();
		return to_wstring(threadNum);
	}

	int64_t getThreadNumber() {
		return _sentenceInfo["text number"];
	}

	string getThreadName() {
		wstring threadNameW = getThreadNameW();
		return StrHelper::convertFromW(threadNameW);
	}

	wstring getThreadNameW() {
		int64_t threadNamePtr = _sentenceInfo["text name"];
		return wstring(reinterpret_cast<wchar_t*>(threadNamePtr));
	}
private:
	SentenceInfo _sentenceInfo;
};
