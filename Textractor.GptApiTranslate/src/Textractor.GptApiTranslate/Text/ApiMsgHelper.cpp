
#include "ApiMsgHelper.h"
#include "../_Libraries/strhelper.h"
#include "../Config/Common.h"

const string DefaultApiMsgHelper::_defaultRequestTemplate = GPT_REQUEST_TEMPLATE;
const string DefaultApiMsgHelper::_defaultResponseMsgPattern = GPT_RESPONSE_MSG_PATTERN;
const string DefaultApiMsgHelper::_defaultErrorMsgPattern = GPT_ERROR_MSG_PATTERN;
const string DefaultApiMsgHelper::_defaultHttpHeaders = GPT_HTTP_HEADERS;


// *** PUBLIC ***

GptConfig DefaultApiMsgHelper::getConfig() const {
	ExtensionConfig cfg = getExtConfig();
	string apiUrl = getApiUrl(cfg);

	string headersStr = cfg.customHttpHeaders.length() ? cfg.customHttpHeaders : _defaultHttpHeaders;
	vector<string> headers = createHttpHeaders(cfg, headersStr);

	return GptConfig(apiUrl, cfg.apiKey, cfg.timeoutSecs, cfg.numRetries, cfg.debugMode, headers);
}

string DefaultApiMsgHelper::getApiUrl(const ExtensionConfig& config) const {
	vector<string> pars = { config.apiKey, config.model };
	return StrHelper::format<char>(config.url, pars);
}

string DefaultApiMsgHelper::createRequestMsg(const string& sysMsg, const string& userMsg) const
{
	ExtensionConfig extConfig = getExtConfig();
	string requestStr = extConfig.customRequestTemplate.length() ? 
		extConfig.customRequestTemplate : _defaultRequestTemplate;

	vector<string> formatPars = { extConfig.model, 
		formatUserMsg(sysMsg), formatUserMsg(userMsg), extConfig.apiKey };
	requestStr = StrHelper::format<char>(requestStr, formatPars);

	return requestStr;
}

string DefaultApiMsgHelper::parseMessageFromResponse(const string& response, bool& error) const
{
	error = false;
	ExtensionConfig extConfig = getExtConfig();
	string responseMsgPattern = extConfig.customResponseMsgRegex.length() ?
		extConfig.customResponseMsgRegex : _defaultResponseMsgPattern;
	
	string msg = parseMessageFromResponse(response, responseMsgPattern);
	if (msg.length()) return msg;

	string errorMsgPattern = extConfig.customErrorMsgRegex.length() ?
		extConfig.customErrorMsgRegex : _defaultErrorMsgPattern;

	msg = parseMessageFromResponse(response, errorMsgPattern);
	if (msg.length()) error = true;
	return msg;
}

bool DefaultApiMsgHelper::hasProcessingError(const string& response) const {
	static const string processingErrMsg = "The server had an error processing your request.";
	return response.find(processingErrMsg) != string::npos;
}


// *** PRIVATE ***

ExtensionConfig DefaultApiMsgHelper::getExtConfig() const {
	return _configRetriever.getConfig(false);
}

vector<string> DefaultApiMsgHelper::createHttpHeaders(
	const ExtensionConfig& config, string headersStr) const
{
	vector<string> pars = { config.apiKey, config.model };
	headersStr = StrHelper::format<char>(headersStr, pars);

	return StrHelper::split(headersStr, HEADERS_DELIM, true);
}

string DefaultApiMsgHelper::formatUserMsg(const string& msg) const {
	string formattedMsg = StrHelper::replace<char>(msg, "\n", "\\n");
	formattedMsg = StrHelper::replace<char>(formattedMsg, "\"", "\\\"");
	return formattedMsg;
}

string DefaultApiMsgHelper::parseMessageFromResponse(const string& response, const string& pattern) const {
	shared_ptr<Regex> regex = getOrSetRegex(pattern);
	vector<string> captures = regex->findMatchCaptures(response);
	if (captures.size() < 2) return "";

	string msg = StrHelper::replace<char>(captures[1], "\\\"", "\"");
	msg = StrHelper::replace<char>(msg, "\\n", "\n");
	return msg;
}

shared_ptr<Regex> DefaultApiMsgHelper::getOrSetRegex(const string& pattern) const {
	if (_regexCache.find(pattern) != _regexCache.end()) return _regexCache.at(pattern);
	shared_ptr<Regex> regex = _regexMap(pattern);
	_regexCache[pattern] = regex;
	return regex;
}
