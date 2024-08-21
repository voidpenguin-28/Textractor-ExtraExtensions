#pragma once

#include "../_Libraries/Locker.h"
#include <string>
#include <unordered_map>
#include <vector>
using namespace std;


class MsgHistoryTracker {
public:
	virtual ~MsgHistoryTracker() { }
	virtual vector<wstring> getFromHistory(int numHistory) const = 0;
	virtual void addToHistory(wstring str) = 0;
};


class MapMsgHistoryTracker : public MsgHistoryTracker {
public:
	static constexpr long MAX_HISTORY_SIZE = 20;

	vector<wstring> getFromHistory(int numHistory) const override {
		vector<wstring> history;
		
		_locker.lock([this, numHistory, &history]() {
			history = getFromHistoryBase(numHistory);
		});

		return history;
	}

	void addToHistory(wstring str) override {
		_locker.lock([this, &str]() {
			addToHistoryBase(str);
		});
	}

private:
	mutable BasicLocker _locker;
	unordered_map<long, wstring> _msgHistory = { };
	long _firstMsgIndex = 0, _lastMsgIndex = -1;

	vector<wstring> getFromHistoryBase(int numHistory) const {
		vector<wstring> subHistory;
		long start = max((_lastMsgIndex - numHistory) + 1, _firstMsgIndex);

		for (long i = start; i <= _lastMsgIndex; i++) {
			subHistory.push_back(_msgHistory.at(i));
		}

		return subHistory;
	}

	void addToHistoryBase(wstring str) {
		_msgHistory[++_lastMsgIndex] = str;
		deleteFromHistory();
	}

	void deleteFromHistory(long maxHistorySize = MAX_HISTORY_SIZE) {
		while (historyMaxed(maxHistorySize)) _msgHistory.erase(_firstMsgIndex++);
	}

	bool historyMaxed(long maxHistorySize = MAX_HISTORY_SIZE) const {
		return _lastMsgIndex - _firstMsgIndex > maxHistorySize;
	}
};
