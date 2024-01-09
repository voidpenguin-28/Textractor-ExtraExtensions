#pragma once

#include "SentenceInfoWrapper.h"
#include <mutex>
#include <unordered_map>
using namespace std;

class ThreadTracker {
public:
	virtual ~ThreadTracker() { }
	virtual size_t trackThreadNameIndex(SentenceInfoWrapper& sentInfoWrap) = 0;
};

class MapThreadTracker : public ThreadTracker {
public:
	size_t trackThreadNameIndex(SentenceInfoWrapper& sentInfoWrap) override {
		lock_guard<mutex> lock(_mutex);
		return trackThreadNameIndexBase(sentInfoWrap);
	}
private:
	const wstring THREAD_ID_DELIM = L":";
	unordered_map<wstring, size_t> _threadNameMap = { };
	unordered_map<wstring, size_t> _threadIdMap = { };
	mutable mutex _mutex;

	size_t trackThreadNameIndexBase(SentenceInfoWrapper& sentInfoWrap) {
		wstring threadName, threadId = createThreadId(sentInfoWrap, threadName);

		if (!mapHasThreadId(threadId))
			_threadIdMap[threadId] = addOrUpdateThreadNameMap(threadName);

		return _threadIdMap[threadId];
	}

	wstring createThreadId(SentenceInfoWrapper& sentInfoWrap, wstring& threadNameOut) const {
		wstring processId = sentInfoWrap.getProcessIdW();
		threadNameOut = sentInfoWrap.getThreadName();
		wstring threadNum = sentInfoWrap.getThreadNumberW();
		wstring delim = THREAD_ID_DELIM;

		wstring threadId = processId + delim + threadNameOut + delim + threadNum;
		return threadId;
	}

	bool mapHasThreadName(const wstring& threadName) const {
		return mapHasKey(_threadNameMap, threadName);
	}

	bool mapHasThreadId(const wstring& threadId) const {
		return mapHasKey(_threadIdMap, threadId);
	}

	bool mapHasKey(const unordered_map<wstring, size_t>& map, const wstring& key) const {
		return map.find(key) != map.end();
	}

	size_t addOrUpdateThreadNameMap(const wstring& threadName) {
		if (!mapHasThreadName(threadName)) _threadNameMap[threadName] = 1;
		else _threadNameMap[threadName]++;

		return _threadNameMap[threadName];
	}
};
