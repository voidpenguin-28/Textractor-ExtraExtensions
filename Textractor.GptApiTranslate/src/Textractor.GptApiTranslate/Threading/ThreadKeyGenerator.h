#pragma once

#include "../Extension.h"
#include <string>

class ThreadKeyGenerator {
public:
	virtual ~ThreadKeyGenerator() { }
	virtual wstring getThreadKey(size_t threadIndex, SentenceInfoWrapper& sentInfoWrapper) const = 0;
};


class DefaultThreadKeyGenerator : public ThreadKeyGenerator {
public:
	wstring getThreadKey(size_t threadIndex, SentenceInfoWrapper& sentInfoWrapper) const override {
		wstring threadKey = sentInfoWrapper.getThreadName() + L"-" + to_wstring(threadIndex);
		return threadKey;
	}
};
