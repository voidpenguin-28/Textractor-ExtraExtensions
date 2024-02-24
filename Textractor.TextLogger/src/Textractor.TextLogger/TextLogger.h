#pragma once

#include "ExtensionConfig.h"
#include "FileWriter.h"
#include "LoggerTextHandle.h"
#include "ThreadKeyGenerator.h"
#include "ThreadFilter.h"
#include "ThreadTracker.h"
#include "SentenceInfoWrapper.h"
using namespace std;

class TextLogger {
public:
	virtual ~TextLogger() { }
	virtual void log(const wstring& sentence, SentenceInfoWrapper& sentInfoWrapper) const = 0;
};

class FileTextLogger : public TextLogger {
public:
	FileTextLogger(const ConfigRetriever& configRetriever, 
		const ThreadKeyGenerator& keyGenerator, const ThreadFilter& threadFilter,
		const LoggerTextHandler& textHandler, ThreadTracker& threadTracker, FileWriterManager& writerManager)
		: _configRetriever(configRetriever), _keyGenerator(keyGenerator), 
			_threadFilter(threadFilter), _textHandler(textHandler),
			_threadTracker(threadTracker), _writerManager(writerManager) { }

	void log(const wstring& sentence, SentenceInfoWrapper& sentInfoWrapper) const override;
private:
	const ConfigRetriever& _configRetriever;
	const ThreadKeyGenerator& _keyGenerator;
	const ThreadFilter& _threadFilter;
	const LoggerTextHandler& _textHandler;
	ThreadTracker& _threadTracker;
	FileWriterManager& _writerManager;

	ExtensionConfig getConfig() const;
	bool meetsConsoleAndClipboardRequirements(
		SentenceInfoWrapper& sentInfoWrapper, const ExtensionConfig& config) const;
	size_t trackThreadIndex(SentenceInfoWrapper& sentInfoWrapper) const;
};
