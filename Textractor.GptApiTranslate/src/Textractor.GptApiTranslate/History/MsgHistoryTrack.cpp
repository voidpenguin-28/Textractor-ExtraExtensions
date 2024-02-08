
#include "MsgHistoryTrack.h"

// *** PUBLIC

vector<wstring> MapMsgHistoryTracker::getFromHistory(int numHistory) const {
	lock_guard<mutex> lock(_mutex);
	return getFromHistoryBase(numHistory);
}

void MapMsgHistoryTracker::addToHistory(wstring str) {
	lock_guard<mutex> lock(_mutex);
	addToHistoryBase(str);
}


// *** PRIVATE

vector<wstring> MapMsgHistoryTracker::getFromHistoryBase(int numHistory) const {
	vector<wstring> subHistory;
	long start = max((_lastMsgIndex - numHistory) + 1, _firstMsgIndex);

	for (long i = start; i <= _lastMsgIndex; i++) {
		subHistory.push_back(_msgHistory.at(i));
	}

	return subHistory;
}

void MapMsgHistoryTracker::addToHistoryBase(wstring str) {
	_msgHistory[++_lastMsgIndex] = str;
	deleteFromHistory();
}


void MapMsgHistoryTracker::deleteFromHistory(long maxHistorySize) {
	while (historyMaxed(maxHistorySize)) _msgHistory.erase(_firstMsgIndex++);
}

bool MapMsgHistoryTracker::historyMaxed(long maxHistorySize) const {
	return _lastMsgIndex - _firstMsgIndex > maxHistorySize;
}
