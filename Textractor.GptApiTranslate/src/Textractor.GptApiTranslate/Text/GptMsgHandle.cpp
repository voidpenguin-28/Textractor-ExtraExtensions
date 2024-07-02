
#include "GptMsgHandle.h"
#include "../_Libraries/strhelper.h"
#include <unordered_set>

const wstring DefaultGptMsgHandler::LINE_SEP = L": ";

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

		finalMsg = to_wstring(msgPrefix) + LINE_SEP + currMsg + L"\n" + finalMsg;
	}

	return StrHelper::rtrim<wchar_t>(finalMsg, L"\n");
}

wstring DefaultGptMsgHandler::getLastTranslationFromResponse(const wstring& str) const {
	size_t startIndex = str.length(), newStartIndex;

	do {
		startIndex = str.rfind(L'\n', startIndex - 1);

		if (startIndex == wstring::npos) {
			startIndex = 0;
			if (!isTransLineStart(str, startIndex, newStartIndex))
				newStartIndex = startIndex;

			break;
		}
	} while (!isTransLineStart(str, startIndex + 1, newStartIndex));

	return str.substr(newStartIndex);
}


// *** PRIVATE

bool DefaultGptMsgHandler::isTransLineStart(const wstring& str, size_t startIndex, size_t& newStartIndex) const {
	static const unordered_set<wchar_t> _seps{ L':', L'.' };
	newStartIndex = wstring::npos;
	int i = 0;
	while (startIndex + i < str.length() && isdigit(str[startIndex + i])) i++;

	if (i == 0 || startIndex + i >= str.length() - 1) return false;
	if (_seps.find(str[startIndex + i]) == _seps.end()) return false;
	if (str[startIndex + ++i] != L' ') return false;

	newStartIndex = startIndex + i + 1;
	return true;
}
