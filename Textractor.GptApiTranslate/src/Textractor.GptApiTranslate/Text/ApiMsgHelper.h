
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
	DefaultApiMsgHelper(const ConfigRetriever& configRetriever, 
		const function<shared_ptr<Regex>(const string& pattern)>& regexMap) 
		: _configRetriever(configRetriever), _regexMap(regexMap) { }

	GptConfig getConfig() const;
	string createRequestMsg(const string& sysMsg, const string& userMsg) const;
	string parseMessageFromResponse(const string& response, bool& error) const;
	bool hasProcessingError(const string& response) const;
private:
	static constexpr char HEADERS_DELIM = '|';
	static const string _defaultRequestTemplate;
	static const string _defaultResponseMsgPattern;
	static const string _defaultErrorMsgPattern;
	static const string _defaultHttpHeaders;

	mutable unordered_map<string, shared_ptr<Regex>> _regexCache{};
	const function<shared_ptr<Regex>(const string& pattern)> _regexMap;
	const ConfigRetriever& _configRetriever;

	ExtensionConfig getExtConfig() const;
	string getApiUrl(const ExtensionConfig& config) const;
	vector<string> createHttpHeaders(const ExtensionConfig& extConfig, string headersStr) const;
	string formatUserMsg(const string& msg) const;
	string parseMessageFromResponse(const string& response, const string& pattern) const;
	shared_ptr<Regex> getOrSetRegex(const string& pattern) const;
};
