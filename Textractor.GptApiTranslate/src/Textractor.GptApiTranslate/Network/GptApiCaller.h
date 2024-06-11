
#pragma once
#include "../Config/ExtensionConfig.h"
#include "../Text/ApiMsgHelper.h"
#include "../Logger.h"
#include "HttpClient.h"
#include <string>
using namespace std;


class GptApiCaller {
public:
	virtual ~GptApiCaller() { }

	virtual pair<bool, string> callCompletionApi(const string& model, 
		const string& sysMsg, const string& userMsg, bool contentOnly = true) const = 0;
};


class DefaultGptApiCaller : public GptApiCaller {
public:
	DefaultGptApiCaller(const HttpClient& httpClient, const Logger& logger, const ApiMsgHelper& msgHelper) 
		: _httpClient(httpClient), _logger(logger), _msgHelper(msgHelper) { }

	pair<bool, string> callCompletionApi(const string& model, const string& sysMsg,
		const string& userMsg, bool contentOnly = true) const override;
private:
	static const string _logFileName;
	const HttpClient& _httpClient;
	const Logger& _logger;
	const ApiMsgHelper& _msgHelper;

	pair<bool, string> callCompletionApi(const GptConfig& config, const string& request) const;
	void writeToLog(const string& request, const string& response, bool error) const;
};
