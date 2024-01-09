#pragma once

#include "ExtensionConfig.h"
#include "SentenceInfoWrapper.h"
using FilterMode = ExtensionConfig::FilterMode;


class ThreadFilter {
public:
	virtual ~ThreadFilter() { }
	virtual bool isThreadAllowed(const wstring& threadKey,
		SentenceInfoWrapper& sentInfoWrapper, const ExtensionConfig& config) const = 0;
};


class NoThreadFilter : public ThreadFilter {
public:
	bool isThreadAllowed(const wstring& threadKey, SentenceInfoWrapper& sentInfoWrapper, 
		const ExtensionConfig& config) const override { return true; }
};

class AllThreadFilter : public ThreadFilter {
public:
	bool isThreadAllowed(const wstring& threadKey, SentenceInfoWrapper& sentInfoWrapper,
		const ExtensionConfig& config) const override { return false; }
};


class DefaultThreadFilter : public ThreadFilter {
public:
	bool isThreadAllowed(const wstring& threadKey, 
		SentenceInfoWrapper& sentInfoWrapper, const ExtensionConfig& config) const override
	{
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
