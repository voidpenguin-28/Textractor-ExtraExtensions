
#pragma once
#include "../_Libraries/strhelper.h"
#include "../Extension.h"
#include "../Config/ExtensionConfig.h"
#include <vector>


class GptMsgCreator {
public:
	virtual ~GptMsgCreator() { }
	virtual wstring createMsg(const ExtensionConfig& config, SentenceInfoWrapper& sentInfoWrapper) = 0;
};


class MultiGptMsgCreator : public GptMsgCreator {
public:
	MultiGptMsgCreator(const vector<reference_wrapper<GptMsgCreator>>& msgCreators)
		: _msgCreators(msgCreators) { }

	wstring createMsg(const ExtensionConfig& config, SentenceInfoWrapper& sentInfoWrapper) override {
		wstring userMsg = L"", currMsg;

		for (auto& creator : _msgCreators) {
			currMsg = creator.get().createMsg(config, sentInfoWrapper);
			if (!currMsg.empty()) userMsg += currMsg + NEW_LINE;
		}

		return StrHelper::rtrim(userMsg, NEW_LINE);
	}
private:
	const wstring NEW_LINE = L"\n";
	vector<reference_wrapper<GptMsgCreator>> _msgCreators;
};
