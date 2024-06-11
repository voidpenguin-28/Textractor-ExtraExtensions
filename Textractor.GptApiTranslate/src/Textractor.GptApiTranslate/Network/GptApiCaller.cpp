
#include "../_Libraries/strhelper.h"
#include "GptApiCaller.h"
#include <fstream>


// *** PUBLIC

pair<bool, string> DefaultGptApiCaller::callCompletionApi(const string& model,
	const string& sysMsg, const string& userMsg, bool contentOnly) const
{
	GptConfig config = _msgHelper.getConfig();
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


// *** PRIVATE

pair<bool, string> DefaultGptApiCaller::callCompletionApi(const GptConfig& config, const string& request) const {
	static const function<bool(const string&)> callRetryCondition =
		[this](const string& r) { return _msgHelper.hasProcessingError(r); };

	string response;
	bool httpError;

	try {
		response = _httpClient.httpPost(config.url, request, 
			config.httpHeaders, config.timeoutSecs, config.numRetries, callRetryCondition);
	}
	catch (exception& ex) {
		response = ex.what();
		httpError = true;
	}

	return pair<bool, string>(httpError, response);
}

void DefaultGptApiCaller::writeToLog(const string& request, const string& response, bool error) const {
	string msg = request + "\n" + response + "\n";
	Logger::Level logLevel = error ? Logger::Error : Logger::Info;

	_logger.log(logLevel, msg);
}
