
#pragma once
#include "ExtensionConfig.h"
#include "Extension.h"


class ExtExecRequirements {
public:
	virtual ~ExtExecRequirements() { }
	virtual bool meetsRequirements(SentenceInfoWrapper& sentInfoWrapper, const ExtensionConfig& config) const = 0;
};


class NoExtExecRequirements : public ExtExecRequirements {
public:
	bool meetsRequirements(SentenceInfoWrapper& sentInfoWrapper, const ExtensionConfig& config) const override {
		return true;
	}
};


class DefaultExtExecRequirements : public ExtExecRequirements {
public:
	bool meetsRequirements(SentenceInfoWrapper& sentInfoWrapper, const ExtensionConfig& config) const override {
		if (config.disabled) return false;
		if (config.activeThreadOnly && !sentInfoWrapper.isActiveThread()) return false;
		if (!meetsConsoleAndClipboardRequirements(sentInfoWrapper, config)) return false;

		return true;
	}
private:
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
};
