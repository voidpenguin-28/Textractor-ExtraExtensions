
#include "GptMsgHandle.h"
#include "../_Libraries/strhelper.h"

const vector<wstring> DefaultGptMsgHandler::LINE_SEPS = { L": ", L". " };
const wregex DefaultGptMsgHandler::_transltSplitPattern(L"(?:^|\n)\\d+(?:" + StrHelper::join(L"|", LINE_SEPS) + L")");


// *** PUBLIC

wstring DefaultGptMsgHandler::createMsgFromHistory(
	const vector<wstring>& msgHist, int msgCharLimit, int histSoftCharLimit) const
{
	wstring finalMsg = L"";
	wstring currMsg;
	int msgLen, msgHistLen = 0, msgChLimit = histSoftCharLimit;
	int msgPrefix = 99, start = ((int)msgHist.size()) - 1;

	for (int i = start; i >= 0; i--, msgPrefix--) {
		currMsg = StrHelper::rtruncate(msgHist[i], msgCharLimit);

		if (msgChLimit > 0) {
			msgLen = currMsg.length();
			if (i < start && (msgHistLen + msgLen) > msgChLimit) break;
			msgHistLen += msgLen;
		}

		finalMsg = to_wstring(msgPrefix) + LINE_SEPS[0] + currMsg + L"\n" + finalMsg;
	}

	return StrHelper::rtrim(finalMsg, L"\n");
}

wstring DefaultGptMsgHandler::getLastTranslationFromResponse(const wstring& responseMsg) const {
	vector<wstring> lines = regSplit(responseMsg);
	wstring lastTranslation = L"";

	for (int i = static_cast<int>(lines.size()) - 1; i >= 0; i--) {
		if (lines[i].empty()) continue;
		lastTranslation = lines[i];
		break;
	}

	return lastTranslation;
}

// *** PRIVATE

vector<wstring> DefaultGptMsgHandler::regSplit(const wstring& str, const wregex& pattern) const {
	wsregex_token_iterator it(str.begin(), str.end(), pattern, -1);
	wsregex_token_iterator end;
	vector<wstring> m;

	while (it != end) {
		m.push_back(*it);
		++it;
	}

	return m;
}
