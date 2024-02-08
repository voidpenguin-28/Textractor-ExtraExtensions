#pragma once

#include "../Extension.h"
#include "../Config/ExtensionConfig.h"
#include "ThreadKeyGenerator.h"
#include "ThreadTracker.h"
using FilterMode = ExtensionConfig::FilterMode;


class ThreadFilter {
public:
	virtual ~ThreadFilter() { }
	virtual bool isThreadAllowed(SentenceInfoWrapper& sentInfoWrapper, const ExtensionConfig& config) const = 0;
};


class NoThreadFilter : public ThreadFilter {
public:
	bool isThreadAllowed(SentenceInfoWrapper& sentInfoWrapper,
		const ExtensionConfig& config) const override {
		return true;
	}
};

class AllThreadFilter : public ThreadFilter {
public:
	bool isThreadAllowed(SentenceInfoWrapper& sentInfoWrapper,
		const ExtensionConfig& config) const override {
		return false;
	}
};


class DefaultThreadFilter : public ThreadFilter {
public:
	DefaultThreadFilter(const ThreadKeyGenerator& keyGenerator, ThreadTracker& threadTracker)
		: _keyGenerator(keyGenerator), _threadTracker(threadTracker) { }

	bool isThreadAllowed(SentenceInfoWrapper& sentInfoWrapper, 
		const ExtensionConfig& config) const override
	{
		size_t threadIndex = _threadTracker.trackThreadNameIndex(sentInfoWrapper);
		wstring threadKey = _keyGenerator.getThreadKey(threadIndex, sentInfoWrapper);
		wstring threadName = sentInfoWrapper.getThreadName();

		switch (config.threadKeyFilterMode) {
		case FilterMode::Blacklist:
			return isInList(threadKey, threadName, config) == false;
		case FilterMode::Whitelist:
			return isInList(threadKey, threadName, config) == true;
		default:
			return true;
		}
	}
private:
	const ThreadKeyGenerator& _keyGenerator;
	ThreadTracker& _threadTracker;

	bool isInList(const wstring& threadKey,
		const wstring& threadName, const ExtensionConfig& config) const
	{
		if (isInList(config.threadKeyFilterList, threadKey, config.threadKeyFilterListDelim)) return true;
		if (isInList(config.threadKeyFilterList, threadName, config.threadKeyFilterListDelim)) return true;

		return false;
	}

	bool isInList(const wstring& list, const wstring& subStr, const wstring& delim) const {
		if (subStr.length() > list.length()) return false;
		wstring searchStr = subStr + delim;
		size_t index = list.find(searchStr);
		if (index != wstring::npos) return true;

		index = list.rfind(subStr);
		if (index == list.length() - subStr.length()) return true;

		return false;
	}
};
