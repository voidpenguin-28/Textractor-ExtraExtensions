#pragma once
#include "../Extension.h"
#include "../Threading/ThreadKeyGenerator.h"
#include "../Threading/ThreadTracker.h"
#include "MsgHistoryTrack.h"
#include <functional>
#include <memory>
#include <mutex>


class MultiThreadMsgHistoryTracker {
public:
	virtual ~MultiThreadMsgHistoryTracker() { }

	virtual vector<wstring> getFromHistory(SentenceInfoWrapper& sentInfoWrapper, int numHistory) = 0;
	virtual void addToHistory(SentenceInfoWrapper& sentInfoWrapper, wstring str) = 0;
};

class DefaultMultiThreadMsgHistoryTracker : public MultiThreadMsgHistoryTracker {
public:
	DefaultMultiThreadMsgHistoryTracker(const ThreadKeyGenerator& keyGenerator, 
		ThreadTracker& threadTracker, const function<MsgHistoryTracker*()> histTrackerGenerator)
		: _keyGenerator(keyGenerator), _threadTracker(threadTracker), 
			_histTrackerGenerator(histTrackerGenerator) { }

	vector<wstring> getFromHistory(SentenceInfoWrapper& sentInfoWrapper, int numHistory) override {
		shared_ptr<MsgHistoryTracker> histTracker = getHistTracker(sentInfoWrapper);
		return histTracker->getFromHistory(numHistory);
	}

	void addToHistory(SentenceInfoWrapper& sentInfoWrapper, wstring str) override {
		shared_ptr<MsgHistoryTracker> histTracker = getHistTracker(sentInfoWrapper);
		return histTracker->addToHistory(str);
	}
private:
	const ThreadKeyGenerator& _keyGenerator;
	ThreadTracker& _threadTracker;
	const function<MsgHistoryTracker*()> _histTrackerGenerator;
	unordered_map<wstring, shared_ptr<MsgHistoryTracker>> _trackerMap;
	mutable mutex _mutex;

	wstring getThreadKey(SentenceInfoWrapper& sentInfoWrapper) const {
		size_t threadIndex = _threadTracker.trackThreadNameIndex(sentInfoWrapper);
		wstring threadKey = _keyGenerator.getThreadKey(threadIndex, sentInfoWrapper);
		return threadKey;
	}

	shared_ptr<MsgHistoryTracker> getHistTracker(SentenceInfoWrapper& sentInfoWrapper) {
		wstring threadKey = getThreadKey(sentInfoWrapper);
		return getHistTracker(threadKey);
	}

	shared_ptr<MsgHistoryTracker> getHistTracker(const wstring& threadKey) {
		lock_guard<mutex> lock(_mutex);
		addHistTrackerIfNotExist(threadKey);
		return _trackerMap[threadKey];
	}

	void addHistTrackerIfNotExist(const wstring& threadKey) {
		if (mapHasKey(threadKey)) return;

		auto tracker = shared_ptr<MsgHistoryTracker>(_histTrackerGenerator());
		_trackerMap[threadKey] = tracker;
	}

	bool mapHasKey(const wstring& threadKey) const {
		return _trackerMap.find(threadKey) != _trackerMap.end();
	}
};
