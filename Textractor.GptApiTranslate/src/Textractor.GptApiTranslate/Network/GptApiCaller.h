
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
	DefaultGptApiCaller(HttpClient& httpClient, const Logger& logger, const ApiMsgHelper& msgHelper) 
		: _httpClient(httpClient), _logger(logger), _msgHelper(msgHelper) { }

	pair<bool, string> callCompletionApi(const string& model,
		const string& sysMsg, const string& userMsg, bool contentOnly) const override
	{
		GptConfig config = _msgHelper.getConfig();

		try {
			string request = _msgHelper.createRequestMsg(sysMsg, userMsg);

			pair<bool, string> output = callCompletionApi(config, request);
			string& response = output.second;
			bool msgError, httpError = output.first;
			string parsedResponse = _msgHelper.parseMessageFromResponse(response, msgError);

			bool anyError = httpError || msgError;
			if (config.logRequest) writeToLog(request, response, anyError);
			if (contentOnly) response = parsedResponse;
			return pair<bool, string>(anyError, response);
		}
		catch (const exception& ex) {
			if (config.logRequest) writeToLog(sysMsg + '\n' + userMsg, ex.what(), true);
			throw;
		}
	}
private:
	static const string _logFileName;
	HttpClient& _httpClient;
	const Logger& _logger;
	const ApiMsgHelper& _msgHelper;

	pair<bool, string> callCompletionApi(const GptConfig& config, const string& request) const {
		static const function<bool(const string&)> callRetryCondition =
			[this](const string& r) { return _msgHelper.hasProcessingError(r); };

		string response;
		bool httpError;

		try {
			response = _httpClient.httpPost(config.url, request,
				config.httpHeaders, config.timeoutSecs, config.numRetries, callRetryCondition);
		}
		catch (const exception& ex) {
			response = ex.what();
			httpError = true;
		}

		return pair<bool, string>(httpError, response);
	}

	void writeToLog(const string& request, const string& response, bool error) const {
		string msg = request + "\n" + response + "\n";
		Logger::Level logLevel = error ? Logger::Error : Logger::Info;

		_logger.log(logLevel, msg);
	}

};
