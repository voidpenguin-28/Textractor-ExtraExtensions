
#include "TextLogger.h"
#include <iostream>
#include <fstream>

// *** PUBLIC

void FileTextLogger::log(const wstring& sentence, SentenceInfoWrapper& sentInfoWrapper) const {
	ExtensionConfig config = getConfig();

	if (config.disabled) return;
	if (config.activeThreadOnly && !sentInfoWrapper.isActiveThread()) return;
	if (!meetsConsoleAndClipboardRequirements(sentInfoWrapper, config)) return;

	size_t threadIndex = trackThreadIndex(sentInfoWrapper);
	wstring threadKey = _keyGenerator.getThreadKey(threadIndex, sentInfoWrapper, config);
	if (!_threadFilter.isThreadAllowed(threadKey, sentInfoWrapper, config)) return;

	wstring logFilePath = _textHandler.getLogFilePath(threadKey, sentInfoWrapper, config);
	wstring msg = _textHandler.createLogMsg(sentence, threadKey, sentInfoWrapper, config);

	_writerManager.write(logFilePath, msg);
}


// *** PRIVATE

ExtensionConfig FileTextLogger::getConfig() const {
	return _configRetriever.getConfig();
}

bool FileTextLogger::meetsConsoleAndClipboardRequirements(
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

size_t FileTextLogger::trackThreadIndex(SentenceInfoWrapper& sentInfoWrapper) const {
	return _threadTracker.trackThreadNameIndex(sentInfoWrapper);
}
