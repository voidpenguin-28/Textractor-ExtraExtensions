#pragma once

#include "Extension.h"
#include <string>
using namespace std;

class SentenceInfoWrapper {
public:
	SentenceInfoWrapper(SentenceInfo& sentenceInfo) : _sentenceInfo(sentenceInfo) { }
	
	bool isActiveThread() {
		return _sentenceInfo["current select"];
	}

	bool threadIsConsoleOrClipboard() {
		return getProcessId() == 0;
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

	wstring getThreadName() {
		int64_t threadNamePtr = _sentenceInfo["text name"];
		return wstring(reinterpret_cast<wchar_t*>(threadNamePtr));
	}
private:
	SentenceInfo _sentenceInfo;
};
