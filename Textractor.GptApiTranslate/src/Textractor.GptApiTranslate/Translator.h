#pragma once

#include "Extension.h"
#include "Config/ExtensionConfig.h"
#include "History/MultiThreadMsgHistoryTrack.h"
#include "Network/GptApiCaller.h"
#include "Text/GptMsgHandle.h"
#include "Text/TranslationFormat.h"
#include "Threading/ThreadFilter.h"
#include <regex> 
#include <string>
#include <vector>
using namespace std;


class Translator {
public:
	~Translator() { }

	virtual string translate(SentenceInfoWrapper& sentInfoWrapper, const wstring& text) const = 0;
	virtual wstring translateW(SentenceInfoWrapper& sentInfoWrapper, const wstring& text) const = 0;
	virtual string translate(SentenceInfoWrapper& sentInfoWrapper, const string& text) const = 0;
	virtual wstring translateW(SentenceInfoWrapper& sentInfoWrapper, const string& text) const = 0;
};


class GptApiTranslator : public Translator {
public:
	GptApiTranslator(const ConfigRetriever& configRetriever, const ThreadFilter& threadFilter,
		MultiThreadMsgHistoryTracker& msgHistTracker, const GptApiCaller& gptApiCaller,
		const GptMsgHandler& gptMsgHandler, const TranslationFormatter& formatter) 
		: _configRetriever(configRetriever), _threadFilter(threadFilter), _msgHistTracker(msgHistTracker),
			_gptApiCaller(gptApiCaller), _gptMsgHandler(gptMsgHandler), _formatter(formatter) { }

	string translate(SentenceInfoWrapper& sentInfoWrapper, const wstring& text) const override;
	wstring translateW(SentenceInfoWrapper& sentInfoWrapper, const wstring& text) const override;
	string translate(SentenceInfoWrapper& sentInfoWrapper, const string& text) const override;
	wstring translateW(SentenceInfoWrapper& sentInfoWrapper, const string& text) const override;
private:
	static const wstring NO_TRNS;
	const ConfigRetriever& _configRetriever;
	const ThreadFilter& _threadFilter;
	MultiThreadMsgHistoryTracker& _msgHistTracker;
	const GptApiCaller& _gptApiCaller;
	const GptMsgHandler& _gptMsgHandler;
	const TranslationFormatter& _formatter;

	bool meetsExecutionRequirements(SentenceInfoWrapper& sentInfoWrapper,
		const wstring& text, const ExtensionConfig& config) const;
	bool isAllAscii(const wstring& str) const;
	bool addJpTextToHistory(SentenceInfoWrapper& sentInfoWrapper, 
		const wstring& text, const ExtensionConfig& config) const;
	string createGptSysMsg(const ExtensionConfig& config) const;
	string createGptUserMsg(SentenceInfoWrapper& sentInfoWrapper, const ExtensionConfig& config) const;
	wstring callGptApi(ExtensionConfig& config, const string& sysMsg, const string& userMsg) const;
};