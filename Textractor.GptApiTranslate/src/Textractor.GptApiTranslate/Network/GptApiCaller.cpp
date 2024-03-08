
#include "GptApiCaller.h"
#include <fstream>

const string DefaultGptApiCaller::_msgTemplate = "{\"model\":\"{0}\",\"messages\":[{\"role\":\"system\",\"content\":\"{1}\"},{\"role\":\"user\",\"content\":\"{2}\"}]}";
const regex DefaultGptApiCaller::_responseMsgPattern = regex("\"[Cc]ontent\":\\s{0,}\"(.{0,})\"$");
const regex DefaultGptApiCaller::_responseErrPattern = regex("\"[Mm]essage\":\\s{0,}\"(.{0,})\",$");
//"content": "\n\nHello there, how may I assist you today?",


// *** PUBLIC

pair<bool, string> DefaultGptApiCaller::callCompletionApi(const string& model,
	const string& sysMsg, const string& userMsg, bool contentOnly) const
{
	GptConfig config = _configGetter();
	string request = createRequestMsg(model, sysMsg, userMsg);
	vector<string> headers = createHeaders(config);

	pair<bool, string> output = callCompletionApi(config, request, headers);
	string& response = output.second;
	bool error = output.first;

	if (config.logRequest) writeToLog(request, response, error);
	if (contentOnly) response = parseMessageFromResponse(response, error);
	return pair<bool, string>(error, response);
}


// *** PRIVATE

string DefaultGptApiCaller::createRequestMsg(const string& model, const string& sysMsg, const string& userMsg) const {
	string msg = _msgTemplate;
	msg = replace(msg, "{0}", model);
	msg = replace(msg, "{1}", formatUserMsg(sysMsg));
	msg = replace(msg, "{2}", formatUserMsg(userMsg));
	return msg;
}

string DefaultGptApiCaller::formatUserMsg(const string& msg) const {
	string formattedMsg = replace(msg, "\n", "\\n");
	formattedMsg = replace(formattedMsg, "\"", "\\\"");
	return formattedMsg;
}

string DefaultGptApiCaller::replace(const string& input, const string& target, const string& replacement) const {
	string result = input;
	size_t startPos = 0;

	while ((startPos = result.find(target, startPos)) != string::npos) {
		result.replace(startPos, target.length(), replacement);
		startPos += replacement.length();
	}

	return result;
}

vector<string> DefaultGptApiCaller::createHeaders(const GptConfig& config) const {
	vector<string> headers = {
		"Content-Type: application/json",
		"Authorization: Bearer " + config.apiKey
	};

	return headers;
}

pair<bool, string> DefaultGptApiCaller::callCompletionApi(
	const GptConfig& config, const string& request, const vector<string>& headers) const
{
	static const function<bool(const string&)> callRetryCondition =
		[this](const string& r) { return hasProcessingError(r); };

	string response;
	bool error;

	try {
		response = _httpClient.httpPost(config.url, request, 
			headers, config.timeoutSecs, config.numRetries, callRetryCondition);

		error = hasAnyError(response);
	}
	catch (exception& ex) {
		response = ex.what();
		error = true;
	}

	return pair<bool, string>(error, response);
}

string DefaultGptApiCaller::parseMessageFromResponse(const string& response, bool error) const {
	smatch match;
	regex pattern = error ? _responseErrPattern : _responseMsgPattern;

	if (!regex_search(response, match, pattern)) return "";
	string msg = replace(match[1].str(), "\\\"", "\"");
	msg = replace(msg, "\\n", "\n");
	return msg;
}

void DefaultGptApiCaller::writeToLog(const string& request, const string& response, bool error) const {
	string msg = request + "\n" + response + "\n";
	Logger::Level logLevel = error ? Logger::Error : Logger::Info;

	_logger.log(logLevel, msg);
}

bool DefaultGptApiCaller::hasAnyError(const string& response) const {
	static const string errKey = "\"error\"";
	return response.find(errKey) != string::npos;
}

bool DefaultGptApiCaller::hasProcessingError(const string& response) const {
	static const string processingErrMsg = "The server had an error processing your request.";
	return response.find(processingErrMsg) != string::npos;
}
