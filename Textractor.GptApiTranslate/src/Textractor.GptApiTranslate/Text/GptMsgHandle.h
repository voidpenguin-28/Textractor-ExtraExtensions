#pragma once

#include <regex>
#include <string>
#include <vector>
using namespace std;

class GptMsgHandler {
public:
	virtual ~GptMsgHandler() { }
	virtual wstring createMsgFromHistory(const vector<wstring>& msgHist, 
		int msgCharLimit = INT_MAX, int histSoftCharLimit = INT_MAX) const = 0;

	virtual wstring getLastTranslationFromResponse(const wstring& responseMsg) const = 0;
};

class DefaultGptMsgHandler : public GptMsgHandler {
public:
	wstring createMsgFromHistory(const vector<wstring>& msgHist,
		int msgCharLimit = INT_MAX, int histSoftCharLimit = INT_MAX) const;

	wstring getLastTranslationFromResponse(const wstring& responseMsg) const;
private:
	static const vector<wstring> LINE_SEPS;
	static const wregex _transltSplitPattern;

	vector<wstring> regSplit(const wstring& str, const wregex& pattern = _transltSplitPattern) const;
};
