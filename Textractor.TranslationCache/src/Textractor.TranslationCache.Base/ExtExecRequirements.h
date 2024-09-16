
#pragma once
#include "ExtensionConfig.h"
#include "Extension.h"
#include "Threading/ThreadFilter.h"


class ExtExecRequirements {
public:
	virtual ~ExtExecRequirements() { }
	virtual bool meetsRequirements(SentenceInfoWrapper& sentInfoWrapper, 
		const ExtensionConfig& config, bool readMode) const = 0;
};


class NoExtExecRequirements : public ExtExecRequirements {
public:
	bool meetsRequirements(SentenceInfoWrapper& sentInfoWrapper, 
		const ExtensionConfig& config, bool readMode) const override
	{
		return true;
	}
};


class DefaultExtExecRequirements : public ExtExecRequirements {
public:
	DefaultExtExecRequirements(const ThreadFilter& threadFilter) : _threadFilter(threadFilter) { }

	bool meetsRequirements(SentenceInfoWrapper& sentInfoWrapper,
		const ExtensionConfig& config, bool readMode) const override
	{
		if (isDisabled(config)) return false;
		if (config.activeThreadOnly && !sentInfoWrapper.isActiveThread()) return false;
		if (!_threadFilter.isThreadAllowed(sentInfoWrapper, config)) return false;
		if (!meetsConsoleAndClipboardRequirements(sentInfoWrapper, config)) return false;

		return true;
	}
private:
	const ThreadFilter& _threadFilter;

	bool isDisabled(const ExtensionConfig& config) const {
		return config.disabledMode == ExtensionConfig::DisabledMode::DisableAll;
	}

	bool meetsConsoleAndClipboardRequirements(
		SentenceInfoWrapper& sentInfoWrapper, const ExtensionConfig& config) const
	{
		static const wstring _consoleThreadName = L"Console";
		static const wstring _clipboardThreadName = L"Clipboard";
		wstring threadName = sentInfoWrapper.getThreadName();

		switch (config.skipConsoleAndClipboard) {
		case ExtensionConfig::ConsoleClipboardMode::SkipAll:
			if (sentInfoWrapper.threadIsConsoleOrClipboard()) return false;
			break;
		case ExtensionConfig::ConsoleClipboardMode::SkipConsole:
			if (threadName == _consoleThreadName) return false;
			break;
		case ExtensionConfig::ConsoleClipboardMode::SkipClipboard:
			if (threadName == _clipboardThreadName) return false;
			break;
		}

		return true;
	}
};
