#pragma once
#include "../Config/ExtensionConfig.h"
#include "../Logger.h"
#include "HttpClient.h"
#include <functional>
#include <regex>
#include <string>
#include <unordered_map>
using namespace std;


class GptApiCaller {
public:
	virtual ~GptApiCaller() { }

	virtual pair<bool, string> callCompletionApi(const string& model, 
		const string& sysMsg, const string& userMsg, bool contentOnly = true) const = 0;
};


class DefaultGptApiCaller : public GptApiCaller {
public:
	struct GptConfig {
		string url;
		string apiKey;
		int timeoutSecs;
		int numRetries;
		bool logRequest;

		GptConfig(string url_, string apiKey_, int timeoutSecs_, int numRetries_, bool logRequest_)
			: url(url_), apiKey(apiKey_), timeoutSecs(timeoutSecs_), logRequest(logRequest_), numRetries(numRetries_) { }
		GptConfig(ExtensionConfig config) : GptConfig(config.url, 
			config.apiKey, config.timeoutSecs, config.numRetries, config.debugMode) { }
	};

	DefaultGptApiCaller(const HttpClient& httpClient, const Logger& logger, const function<GptConfig()> configGetter) 
		: _httpClient(httpClient), _logger(logger), _configGetter(configGetter) { }

	pair<bool, string> callCompletionApi(const string& model, const string& sysMsg,
		const string& userMsg, bool contentOnly = true) const override;
private:
	static const string _logFileName;
	static const string _msgTemplate;
	static const regex _responseMsgPattern;
	static const regex _responseErrPattern;

	const HttpClient& _httpClient;
	const Logger& _logger;
	const function<GptConfig()> _configGetter;

	string createRequestMsg(const string& model, const string& sysMsg, const string& userMsg) const;
	string formatUserMsg(const string& msg) const;
	string replace(const string& input, const string& target, const string& replacement) const;
	vector<string> createHeaders(const GptConfig& config) const;
	pair<bool, string> callCompletionApi(const GptConfig& config, 
		const string& request, const vector<string>& headers) const;
	string parseMessageFromResponse(const string& response, bool error) const;
	void writeToLog(const string& request, const string& response, bool error) const;
	bool hasAnyError(const string& response) const;
	bool hasProcessingError(const string& response) const;
};
