
#pragma once
#include "GptMsgCreator.h"
#include "../History/MultiThreadMsgHistoryTrack.h"


class DefaultUserGptMsgCreator : public GptMsgCreator {
public:
	wstring createMsg(const ExtensionConfig& config, SentenceInfoWrapper& sentInfoWrapper) override {
		return config.userMsgPrefix;
	}
};

class MsgHistoryUserGptMsgCreator : public GptMsgCreator {
public:
	MsgHistoryUserGptMsgCreator(MultiThreadMsgHistoryTracker& msgHistTracker) 
		: _msgHistTracker(msgHistTracker) { }

	wstring createMsg(const ExtensionConfig& config, SentenceInfoWrapper& sentInfoWrapper) override {
		int msgHistCount = !config.useHistoryForNonActiveThreads && !sentInfoWrapper.isActiveThread() ? 0 : config.msgHistoryCount;
		vector<wstring> msgHist = _msgHistTracker.getFromHistory(sentInfoWrapper, msgHistCount + 1);
		return createMsgFromHistory(msgHist, config.msgCharLimit, config.historySoftCharLimit);
	}
private:
	const wstring LINE_SEP = L": ";
	MultiThreadMsgHistoryTracker& _msgHistTracker;

	wstring createMsgFromHistory(const vector<wstring>& msgHist, int msgCharLimit, int histSoftCharLimit) {
		wstring finalMsg = L"";
		wstring currMsg;
		int msgLen, msgHistLen = 0, msgChLimit = histSoftCharLimit;
		int msgPrefix = 99, start = ((int)msgHist.size()) - 1;

		for (int i = start; i >= 0; i--, msgPrefix--) {
			currMsg = StrHelper::rtruncate(msgHist[i], msgCharLimit);

			if (msgChLimit > 0) {
				msgLen = (int)currMsg.length();
				if (i < start && (msgHistLen + msgLen) > msgChLimit) break;
				msgHistLen += msgLen;
			}

			finalMsg = to_wstring(msgPrefix) + LINE_SEP + currMsg + L"\n" + finalMsg;
		}

		return StrHelper::rtrim<wchar_t>(finalMsg, L"\n");
	}
};
