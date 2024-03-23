
#pragma once

#include "Extension.h"
#include <string>
using namespace std;


class ThreadIdGenerator {
public:
	virtual ~ThreadIdGenerator() { }
	virtual string generateId(SentenceInfoWrapper& sentInfoWrapper) = 0;
};


class DefaultThreadIdGenerator : public ThreadIdGenerator {
	string generateId(SentenceInfoWrapper& sentInfoWrapper) {
		int64_t threadNum = sentInfoWrapper.getThreadNumber();
		string threadName = sentInfoWrapper.getThreadName();

		return to_string(threadNum) + "_" + threadName;
	}
};
