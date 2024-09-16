#pragma once

#include "../_Libraries/Locker.h"
#include "../Extension.h"
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
		wstring threadName, threadId = createThreadId(sentInfoWrap, threadName);
		return trackThreadNameIndexBase(threadId, threadName);
	}
private:
	const wstring THREAD_ID_DELIM = L":";
	unordered_map<wstring, size_t> _threadNameMap = { };
	unordered_map<wstring, size_t> _threadIdMap = { };
	mutable DefaultLockerMap<wstring> _lockerMap;
	
	size_t trackThreadNameIndexBase(const wstring& threadId, const wstring& threadName) {
		size_t index = 0;

		_lockerMap.getOrCreateLocker(threadId).lock([this, &threadId, &threadName, &index]() {
			if (!mapHasThreadId(threadId))
				_threadIdMap[threadId] = addOrUpdateThreadNameMap(threadName);

			index = _threadIdMap[threadId];
		});

		return index;
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
