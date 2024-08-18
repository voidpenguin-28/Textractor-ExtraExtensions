#pragma once

#include "../Extension.h"
#include "../ExtensionConfig.h"
#include <string>

class ThreadKeyGenerator {
public:
	virtual ~ThreadKeyGenerator() { }
	virtual wstring getThreadKey(size_t threadIndex,
		SentenceInfoWrapper& sentInfoWrapper, const ExtensionConfig& config) const = 0;
};


class DefaultThreadKeyGenerator : public ThreadKeyGenerator {
public:
	wstring getThreadKey(size_t threadIndex,
		SentenceInfoWrapper& sentInfoWrapper, const ExtensionConfig& config) const override
	{
		wstring threadKey = sentInfoWrapper.getThreadName();
		if (!config.onlyThreadNameAsKey) threadKey += L"-" + to_wstring(threadIndex);
		return threadKey;
	}
};
