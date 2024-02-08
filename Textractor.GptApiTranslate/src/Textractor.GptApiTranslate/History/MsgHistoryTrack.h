#pragma once

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
using namespace std;


class MsgHistoryTracker {
public:
	~MsgHistoryTracker() { }
	virtual vector<wstring> getFromHistory(int numHistory) const = 0;
	virtual void addToHistory(wstring str) = 0;
};


class MapMsgHistoryTracker : public MsgHistoryTracker {
public:
	static constexpr long MAX_HISTORY_SIZE = 20;

	vector<wstring> getFromHistory(int numHistory) const override;
	void addToHistory(wstring str) override;
private:
	mutable mutex _mutex;
	unordered_map<long, wstring> _msgHistory = { };
	long _firstMsgIndex = 0, _lastMsgIndex = -1;

	vector<wstring> getFromHistoryBase(int numHistory) const;
	void addToHistoryBase(wstring str);
	void deleteFromHistory(long maxHistorySize = MAX_HISTORY_SIZE);
	bool historyMaxed(long maxHistorySize = MAX_HISTORY_SIZE) const;
};
