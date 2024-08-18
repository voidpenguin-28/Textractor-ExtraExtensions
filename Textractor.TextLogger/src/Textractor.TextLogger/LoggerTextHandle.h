#pragma once

#include "_Libraries/datetime.h"
#include "_Libraries/strhelper.h"
#include "Extension.h"
#include "ExtensionConfig.h"
#include "ProcessNameRetriever.h"
#include <string>
using namespace std;


class LoggerTextHandler {
public:
	virtual ~LoggerTextHandler() { }

	virtual wstring getLogFilePath(const wstring& threadKey, 
		SentenceInfoWrapper& sentInfoWrapper, const ExtensionConfig& config) const = 0;
	virtual wstring createLogMsg(const wstring& sentence, const wstring& threadKey,
		SentenceInfoWrapper& sentInfoWrapper, const ExtensionConfig& config) const = 0;
};


class DefaultLoggerTextHandler : public LoggerTextHandler {
public:
	DefaultLoggerTextHandler(const ProcessNameRetriever& procNameRetriever) 
		: _procNameRetriever(procNameRetriever) { }

	wstring getLogFilePath(const wstring& threadKey,
		SentenceInfoWrapper& sentInfoWrapper, const ExtensionConfig& config) const override
	{
		static const wstring PROCESS_NAME_PLACEHOLDER = L"{0}";
		static const wstring THREAD_KEY_PLACEHOLDER = L"{1}";
		static const wstring THREAD_NAME_PLACEHOLDER = L"{2}";

		wstring processName = getProcessName(sentInfoWrapper);
		wstring threadName = sentInfoWrapper.getThreadName();
		
		wstring logFilePath = config.logFilePathTemplate;
		logFilePath = replace(logFilePath, PROCESS_NAME_PLACEHOLDER, processName);
		logFilePath = replace(logFilePath, THREAD_KEY_PLACEHOLDER, threadKey);
		logFilePath = replace(logFilePath, THREAD_NAME_PLACEHOLDER, threadName);

		return logFilePath;
	}

	wstring createLogMsg(const wstring& sentence, const wstring& threadKey,
		SentenceInfoWrapper& sentInfoWrapper, const ExtensionConfig& config) const override
	{
		static const wstring SENTENCE_PLACEHOLDER = L"{0}";
		static const wstring PROC_NAME_PLACEHOLDER = L"{1}";
		static const wstring THREAD_KEY_PLACEHOLDER = L"{2}";
		static const wstring THREAD_NAME_PLACEHOLDER = L"{3}";
		static const wstring DATETIME_PLACEHOLDER = L"{4}";

		wstring msg = config.msgTemplate;
		if (msg.empty()) msg = SENTENCE_PLACEHOLDER;

		if (found(msg, PROC_NAME_PLACEHOLDER)) {
			wstring procName = getProcessName(sentInfoWrapper);
			msg = replace(msg, PROC_NAME_PLACEHOLDER, procName);
		}
		if (found(msg, THREAD_KEY_PLACEHOLDER)) {
			msg = replace(msg, THREAD_KEY_PLACEHOLDER, threadKey);
		}
		if (found(msg, THREAD_NAME_PLACEHOLDER)) {
			wstring threadName = sentInfoWrapper.getThreadName();
			msg = replace(msg, THREAD_NAME_PLACEHOLDER, threadName);
		}
		if (found(msg, DATETIME_PLACEHOLDER)) {
			wstring currDateTime = StrHelper::convertToW(getCurrentDateTime());
			msg = replace(msg, DATETIME_PLACEHOLDER, currDateTime);
		}
		if (found(msg, SENTENCE_PLACEHOLDER)) {
			msg = replace(msg, SENTENCE_PLACEHOLDER, sentence);
		}

		return msg;
	}
private:
	const ProcessNameRetriever& _procNameRetriever;

	wstring getProcessName(SentenceInfoWrapper& sentInfoWrapper) const {
		DWORD pid = sentInfoWrapper.getProcessIdD();
		return _procNameRetriever.getProcessName(pid);
	}

	wstring replace(wstring str, const wstring& target, const wstring& replacement) const {
		size_t startPos = 0;

		while ((startPos = str.find(target, startPos)) != wstring::npos) {
			str.replace(startPos, target.length(), replacement);
			startPos += replacement.length();
		}

		return str;
	}

	bool found(const wstring& str, const wstring& subStr) const {
		return str.find(subStr) != wstring::npos;
	}
};
