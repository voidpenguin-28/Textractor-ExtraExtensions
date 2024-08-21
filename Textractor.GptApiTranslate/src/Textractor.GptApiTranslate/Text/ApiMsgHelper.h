
#pragma once
#include "../Config/ExtensionConfig.h"
#include "../_Libraries/regex/Regex.h"
#include <functional>
#include <memory>


class ApiMsgHelper {
public:
	virtual ~ApiMsgHelper() { }
	virtual GptConfig getConfig() const = 0;
	virtual string createRequestMsg(const string& sysMsg, const string& userMsg) const = 0;
	virtual string parseMessageFromResponse(const string& response, bool& error) const = 0;
	virtual bool hasProcessingError(const string& response) const = 0;
};


class DefaultApiMsgHelper : public ApiMsgHelper {
public:
	DefaultApiMsgHelper(ConfigRetriever& configRetriever, 
		const function<shared_ptr<Regex>(const string& pattern)>& regexMap) 
		: _configRetriever(configRetriever), _regexMap(regexMap) { }

	GptConfig getConfig() const override {
		ExtensionConfig cfg = getExtConfig();
		string apiUrl = getApiUrl(cfg);

		string headersStr = cfg.customHttpHeaders.length() ? cfg.customHttpHeaders : _defaultHttpHeaders;
		vector<string> headers = createHttpHeaders(cfg, headersStr);

		return GptConfig(apiUrl, cfg.apiKey, cfg.timeoutSecs, cfg.numRetries, cfg.debugMode, headers);
	}

	string createRequestMsg(const string& sysMsg, const string& userMsg) const override
	{
		ExtensionConfig extConfig = getExtConfig();
		string requestStr = extConfig.customRequestTemplate.length() ?
			extConfig.customRequestTemplate : _defaultRequestTemplate;

		vector<string> formatPars = { extConfig.model,
			formatUserMsg(sysMsg), formatUserMsg(userMsg), extConfig.apiKey };
		requestStr = StrHelper::format<char>(requestStr, formatPars);

		return requestStr;
	}

	string parseMessageFromResponse(const string& response, bool& error) const override
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

	bool hasProcessingError(const string& response) const override {
		static const string processingErrMsg = "The server had an error processing your request.";
		return response.find(processingErrMsg) != string::npos;
	}
private:
	static constexpr char HEADERS_DELIM = '|';
	const string _defaultRequestTemplate = GPT_REQUEST_TEMPLATE;
	const string _defaultResponseMsgPattern = GPT_RESPONSE_MSG_PATTERN;
	const string _defaultErrorMsgPattern = GPT_ERROR_MSG_PATTERN;
	const string _defaultHttpHeaders = GPT_HTTP_HEADERS;

	mutable unordered_map<string, shared_ptr<Regex>> _regexCache{};
	const function<shared_ptr<Regex>(const string& pattern)> _regexMap;
	ConfigRetriever& _configRetriever;

	ExtensionConfig getExtConfig() const {
		return _configRetriever.getConfig(false);
	}

	string getApiUrl(const ExtensionConfig& config) const {
		vector<string> pars = { config.apiKey, config.model };
		return StrHelper::format<char>(config.url, pars);
	}

	vector<string> createHttpHeaders(
		const ExtensionConfig& config, string headersStr) const
	{
		vector<string> pars = { config.apiKey, config.model };
		headersStr = StrHelper::format<char>(headersStr, pars);

		return StrHelper::split(headersStr, HEADERS_DELIM, true);
	}

	string formatUserMsg(const string& msg) const {
		string formattedMsg = StrHelper::replace<char>(msg, "\n", "\\n");
		formattedMsg = StrHelper::replace<char>(formattedMsg, "\"", "\\\"");
		return formattedMsg;
	}

	string parseMessageFromResponse(const string& response, const string& pattern) const {
		shared_ptr<Regex> regex = getOrSetRegex(pattern);
		vector<string> captures = regex->findMatchCaptures(response);
		if (captures.size() < 2) return "";

		string msg = StrHelper::replace<char>(captures[1], "\\\"", "\"");
		msg = StrHelper::replace<char>(msg, "\\n", "\n");
		return msg;
	}

	shared_ptr<Regex> getOrSetRegex(const string& pattern) const {
		if (_regexCache.find(pattern) != _regexCache.end()) return _regexCache.at(pattern);
		shared_ptr<Regex> regex = _regexMap(pattern);
		_regexCache[pattern] = regex;
		return regex;
	}
};
