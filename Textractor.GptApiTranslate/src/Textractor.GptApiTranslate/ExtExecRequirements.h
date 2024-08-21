
#pragma once
#include "Extension.h"
#include "Config/ExtensionConfig.h"
#include "Threading/ThreadFilter.h"


class ExtExecRequirements {
public:
	virtual ~ExtExecRequirements() { }
	virtual bool meetsRequirements(SentenceInfoWrapper& sentInfoWrapper, 
		const ExtensionConfig& config, const wstring& text) const = 0;
};


class NoExtExecRequirements : public ExtExecRequirements {
public:
	bool meetsRequirements(SentenceInfoWrapper& sentInfoWrapper, 
		const ExtensionConfig& config, const wstring& text) const override
	{
		return true;
	}
};


class DefaultExtExecRequirements : public ExtExecRequirements {
public:
	DefaultExtExecRequirements(const ThreadFilter& threadFilter) : _threadFilter(threadFilter) { }

	bool meetsRequirements(SentenceInfoWrapper& sentInfoWrapper, 
		const ExtensionConfig& config, const wstring& text) const override
	{
		if (config.disabled) return false;
		if (config.activeThreadOnly && !sentInfoWrapper.isActiveThread()) return false;
		if (!_threadFilter.isThreadAllowed(sentInfoWrapper, config)) return false;
		if (!meetsConsoleAndClipboardRequirements(sentInfoWrapper, config)) return false;
		if (config.skipAsciiText && isAllAscii(text)) return false;

		return true;
	}
private:
	const ThreadFilter& _threadFilter;

	bool meetsConsoleAndClipboardRequirements(
		SentenceInfoWrapper& sentInfoWrapper, const ExtensionConfig& config) const
	{
		static const wstring _consoleThreadName = L"Console";
		static const wstring _clipboardThreadName = L"Clipboard";
		wstring threadName = sentInfoWrapper.getThreadNameW();

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

	bool isAllAscii(const wstring& str) const {
		static constexpr int ASCII_END_CODE = 127;

		for (size_t i = 0; i < str.length(); i++) {
			if ((int)str[i] > ASCII_END_CODE) return false;
		}

		return true;
	}
};
