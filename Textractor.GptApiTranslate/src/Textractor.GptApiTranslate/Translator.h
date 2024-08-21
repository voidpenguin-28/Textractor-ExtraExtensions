#pragma once

#include "Extension.h"
#include "ExtExecRequirements.h"
#include "Config/ExtensionConfig.h"
#include "History/MultiThreadMsgHistoryTrack.h"
#include "Network/GptApiCaller.h"
#include "Text/GptSysMsgCreator.h"
#include "Text/GptUserMsgCreator.h"
#include "Text/GptLineParser.h"
#include "Text/TranslationFormat.h"
#include "Threading/ThreadFilter.h"
#include <string>
using namespace std;


class Translator {
public:
	virtual ~Translator() { }
	virtual string translate(SentenceInfoWrapper& sentInfoWrapper, const wstring& text) const = 0;
	virtual wstring translateW(SentenceInfoWrapper& sentInfoWrapper, const wstring& text) const = 0;
	virtual string translate(SentenceInfoWrapper& sentInfoWrapper, const string& text) const = 0;
	virtual wstring translateW(SentenceInfoWrapper& sentInfoWrapper, const string& text) const = 0;
};


class GptApiTranslator : public Translator {
public:
	GptApiTranslator(ConfigRetriever& configRetriever, ExtExecRequirements& execRequirements,
		const ThreadFilter& threadFilter, MultiThreadMsgHistoryTracker& msgHistTracker, 
		const GptApiCaller& gptApiCaller, const TranslationFormatter& formatter, GptMsgCreator& sysMsgCreator, 
		GptMsgCreator& userMsgCreator, const GptLineParser& gptLineParser)
		: _configRetriever(configRetriever), _execRequirements(execRequirements), _threadFilter(threadFilter),
			_msgHistTracker(msgHistTracker), _gptApiCaller(gptApiCaller), _formatter(formatter), 
			_sysMsgCreator(sysMsgCreator), _userMsgCreator(userMsgCreator), _gptLineParser(gptLineParser) { }

	wstring translateW(SentenceInfoWrapper& sentInfoWrapper, const string& text) const override {
		string translation = translate(sentInfoWrapper, text);
		return StrHelper::convertToW(translation);
	}

	string translate(SentenceInfoWrapper& sentInfoWrapper, const string& text) const override {
		return translate(sentInfoWrapper, StrHelper::convertToW(text));
	}

	string translate(SentenceInfoWrapper& sentInfoWrapper, const wstring& text) const override {
		wstring translation = translateW(sentInfoWrapper, text);
		return StrHelper::convertFromW(translation);
	}

	wstring translateW(SentenceInfoWrapper& sentInfoWrapper, const wstring& text) const override {
		if (text.empty()) return L"";
		ExtensionConfig config = _configRetriever.getConfig(true);

		if (!_execRequirements.meetsRequirements(sentInfoWrapper, config, text)) return NO_TRNS;
		if (!addJpTextToHistory(sentInfoWrapper, text, config)) return NO_TRNS;

		string sysMsg = StrHelper::convertFromW(_sysMsgCreator.createMsg(config, sentInfoWrapper));
		string userMsg = StrHelper::convertFromW(_userMsgCreator.createMsg(config, sentInfoWrapper));
		wstring translation = callGptApi(config, sysMsg, userMsg);

		translation = _gptLineParser.parseLastLine(translation);
		return _formatter.formatTranslation(translation);
	}

private:
	const wstring NO_TRNS = L"";

	ConfigRetriever& _configRetriever;
	ExtExecRequirements& _execRequirements;
	const ThreadFilter& _threadFilter;
	MultiThreadMsgHistoryTracker& _msgHistTracker;
	const GptApiCaller& _gptApiCaller;
	const TranslationFormatter& _formatter;
	GptMsgCreator& _sysMsgCreator;
	GptMsgCreator& _userMsgCreator;
	const GptLineParser& _gptLineParser;

	// Should return bool for whether or not to continue gpt request
	bool addJpTextToHistory(SentenceInfoWrapper& sentInfoWrapper,
		const wstring& text, const ExtensionConfig& config) const
	{
		wstring formattedText = _formatter.formatJp(text);
		size_t zeroWidthSpaceIndex = formattedText.find(ZERO_WIDTH_SPACE);

		if (zeroWidthSpaceIndex != wstring::npos) {
			_msgHistTracker.addToHistory(sentInfoWrapper, formattedText.substr(0, zeroWidthSpaceIndex));
			if (config.skipIfZeroWidthSpace) return false;
		}
		else {
			_msgHistTracker.addToHistory(sentInfoWrapper, formattedText);
		}

		return true;
	}

	wstring callGptApi(ExtensionConfig& config, const string& sysMsg, const string& userMsg) const {
		pair<bool, string> output = _gptApiCaller.callCompletionApi(config.model, sysMsg, userMsg, true);
		bool error = output.first;

		string translation = output.second;
		if (error) translation = config.showErrMsg ? "*** API ERROR:\n" + translation : "";

		return StrHelper::convertToW(translation);
	}
};