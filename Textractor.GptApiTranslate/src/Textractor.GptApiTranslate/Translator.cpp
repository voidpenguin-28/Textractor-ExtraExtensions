﻿
#include "Translator.h"
#include "_Libraries/strhelper.h"

const wstring GptApiTranslator::NO_TRNS = L"";

// *** PUBLIC

wstring GptApiTranslator::translateW(SentenceInfoWrapper& sentInfoWrapper, const string& text) const {
	string translation = translate(sentInfoWrapper, text);
	return StrHelper::convertToW(translation);
}

string GptApiTranslator::translate(SentenceInfoWrapper& sentInfoWrapper, const string& text) const {
	return translate(sentInfoWrapper, StrHelper::convertToW(text));
}

string GptApiTranslator::translate(SentenceInfoWrapper& sentInfoWrapper, const wstring& text) const {
	wstring translation = translateW(sentInfoWrapper, text);
	return StrHelper::convertFromW(translation);
}

wstring GptApiTranslator::translateW(SentenceInfoWrapper& sentInfoWrapper, const wstring& text) const {
	if (text.empty()) return L"";
	ExtensionConfig config = _configRetriever.getConfig(true);

	if (!meetsExecutionRequirements(sentInfoWrapper, text, config)) return NO_TRNS;
	if (!addJpTextToHistory(sentInfoWrapper, text, config)) return NO_TRNS;

	string sysMsg = StrHelper::convertFromW(_sysMsgCreator.createMsg(config, sentInfoWrapper));
	string userMsg = StrHelper::convertFromW(_userMsgCreator.createMsg(config, sentInfoWrapper));
	wstring translation = callGptApi(config, sysMsg, userMsg);

	translation = _gptLineParser.parseLastLine(translation);
	return _formatter.formatTranslation(translation);
}


// *** PRIVATE

bool GptApiTranslator::meetsExecutionRequirements(SentenceInfoWrapper& sentInfoWrapper,
	const wstring& text, const ExtensionConfig& config) const
{
	if (config.disabled) return false;
	if (config.activeThreadOnly && !sentInfoWrapper.isActiveThread()) return false;
	if (!_threadFilter.isThreadAllowed(sentInfoWrapper, config)) return false;
	if (!meetsConsoleAndClipboardRequirements(sentInfoWrapper, config)) return false;

	if (config.skipAsciiText && isAllAscii(text)) return false;

	return true;
}

bool GptApiTranslator::meetsConsoleAndClipboardRequirements(
	SentenceInfoWrapper& sentInfoWrapper, const ExtensionConfig& config) const
{
	wstring threadName = sentInfoWrapper.getThreadName();

	switch (config.skipConsoleAndClipboard) {
	case 1:
		if (sentInfoWrapper.threadIsConsoleOrClipboard()) return false;
		break;
	case 2:
		if (threadName == L"Console") return false;
		break;
	case 3:
		if (threadName == L"Clipboard") return false;
		break;
	}

	return true;
}

bool GptApiTranslator::isAllAscii(const wstring& str) const {
	static constexpr int ASCII_END_CODE = 127;

	for (size_t i = 0; i < str.length(); i++) {
		if ((int)str[i] > ASCII_END_CODE) return false;
	}

	return true;
}

// Should return bool for whether or not to continue gpt request
bool GptApiTranslator::addJpTextToHistory(SentenceInfoWrapper& sentInfoWrapper, 
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

wstring GptApiTranslator::callGptApi(ExtensionConfig& config, const string& sysMsg, const string& userMsg) const {
	pair<bool, string> output = _gptApiCaller.callCompletionApi(config.model, sysMsg, userMsg, true);
	bool error = output.first;

	string translation = output.second;
	if (error) translation = config.showErrMsg ? "*** API ERROR:\n" + translation : "";

	return StrHelper::convertToW(translation);
}
