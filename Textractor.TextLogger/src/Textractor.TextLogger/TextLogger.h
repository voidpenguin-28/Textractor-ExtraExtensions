#pragma once

#include "_Libraries/strhelper.h"
#include "Extension.h"
#include "ExtensionConfig.h"
#include "ExtExecRequirements.h"
#include "LoggerTextHandle.h"
#include "File/Writer/FileWriter.h"
#include "Threading/ThreadKeyGenerator.h"
#include "Threading/ThreadFilter.h"
#include "Threading/ThreadTracker.h"
using namespace std;

class TextLogger {
public:
	virtual ~TextLogger() { }
	virtual void log(const wstring& sentence, SentenceInfoWrapper& sentInfoWrapper) = 0;
};

class FileTextLogger : public TextLogger {
public:
	FileTextLogger(ConfigRetriever& configRetriever, 
		const ThreadKeyGenerator& keyGenerator, const LoggerTextHandler& textHandler, 
		ThreadTracker& threadTracker, FileWriter& fileWriter, ExtExecRequirements& execRequirements)
		: _configRetriever(configRetriever), _keyGenerator(keyGenerator), 
			_textHandler(textHandler), _threadTracker(threadTracker), 
			_fileWriter(fileWriter), _execRequirements(execRequirements) { }

	void log(const wstring& sentence, SentenceInfoWrapper& sentInfoWrapper) override {
		ExtensionConfig config = getConfig();
		if (!_execRequirements.meetsRequirements(sentInfoWrapper, config)) return;

		wstring threadKey = createThreadKey(sentInfoWrapper, config);
		wstring logFilePath = _textHandler.getLogFilePath(threadKey, sentInfoWrapper, config);
		wstring msg = _textHandler.createLogMsg(sentence, threadKey, sentInfoWrapper, config);

		appendToFile(logFilePath, msg);
	}
private:
	ConfigRetriever& _configRetriever;
	const ThreadKeyGenerator& _keyGenerator;
	const LoggerTextHandler& _textHandler;
	ThreadTracker& _threadTracker;
	FileWriter& _fileWriter;
	ExtExecRequirements& _execRequirements;

	ExtensionConfig getConfig() {
		return _configRetriever.getConfig();
	}

	wstring createThreadKey(SentenceInfoWrapper& sentInfoWrapper, const ExtensionConfig& config) {
		size_t threadIndex = _threadTracker.trackThreadNameIndex(sentInfoWrapper);
		wstring threadKey = _keyGenerator.getThreadKey(threadIndex, sentInfoWrapper, config);
		return threadKey;
	}

	void appendToFile(const wstring& logFilePath, const wstring& msg) {
		_fileWriter.appendToFile(
			StrHelper::convertFromW(logFilePath), StrHelper::convertFromW(msg));
	}
};
