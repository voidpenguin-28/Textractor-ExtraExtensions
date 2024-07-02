#pragma once

#include "../_Libraries/curlproc.h"
#include <functional>
#include <string>
#include <vector>
using namespace std;

class HttpClient {
public:
	virtual ~HttpClient() { }
	static constexpr int DEFAULT_CONNECT_TIMEOUT_SECS = 10;
	static constexpr int DEFAULT_NUM_RETRIES = 0;

	virtual string httpGet(const string& url, const vector<string>& headers = vector<string>(), 
		int connectTimeoutSecs = DEFAULT_CONNECT_TIMEOUT_SECS, int numRetries = DEFAULT_NUM_RETRIES,
		const function<bool(const string&)>& customRetryCondition = {}) const = 0;
	virtual string httpPost(const string& url, const string& body, const vector<string>& headers = vector<string>(),
		int connectTimeoutSecs = DEFAULT_CONNECT_TIMEOUT_SECS, int numRetries = DEFAULT_NUM_RETRIES,
		const function<bool(const string&)>& customRetryCondition = {}) const = 0;
};

class BasicStubHttpClient : public HttpClient {
public:
	BasicStubHttpClient(const string& getResponse, const string& postResponse)
		: _getResponse(getResponse), _postResponse(postResponse) { }
	BasicStubHttpClient(const string& response) : _getResponse(response), _postResponse(response) { }

	string httpGet(const string& url, const vector<string>& headers = vector<string>(),
		int connectTimeoutSecs = DEFAULT_CONNECT_TIMEOUT_SECS, int numRetries = DEFAULT_NUM_RETRIES,
		const function<bool(const string&)>& customRetryCondition = {}) const
	{
		return _getResponse;
	}

	string httpPost(const string& url, const string& body, const vector<string>& headers = vector<string>(),
		int connectTimeoutSecs = DEFAULT_CONNECT_TIMEOUT_SECS, int numRetries = DEFAULT_NUM_RETRIES,
		const function<bool(const string&)>& customRetryCondition = {}) const
	{
		return _postResponse;
	}
private:
	const string _getResponse;
	const string _postResponse;
};

class CurlProcHttpClient : public HttpClient {
public:
	CurlProcHttpClient(const function<string()> customCurlPathGetter)
		: _customCurlPathGetter(customCurlPathGetter) { }
	CurlProcHttpClient(const string& customCurlPath)
		: CurlProcHttpClient([customCurlPath]() { return customCurlPath; }) { }

	string httpGet(const string& url, const vector<string>& headers = vector<string>(),
		int connectTimeoutSecs = DEFAULT_CONNECT_TIMEOUT_SECS, int numRetries = DEFAULT_NUM_RETRIES,
		const function<bool(const string&)>& customRetryCondition = {}) const override
	{
		string customCurlPath = _customCurlPathGetter();

		return executeWithRetry(numRetries, [&url, &headers, connectTimeoutSecs, &customCurlPath]() {
			return curlproc::httpGet(url, headers,
				connectTimeoutSecs, curlproc::DEFAULT_USER_AGENT, customCurlPath);
		}, customRetryCondition);
	}

	string httpPost(const string& url, const string& body, const vector<string>& headers = vector<string>(),
		int connectTimeoutSecs = DEFAULT_CONNECT_TIMEOUT_SECS, int numRetries = DEFAULT_NUM_RETRIES,
		const function<bool(const string&)>& customRetryCondition = {}) const override
	{
		string customCurlPath = _customCurlPathGetter();

		return executeWithRetry(numRetries, [&url, &body, &headers, connectTimeoutSecs, &customCurlPath]() {
			return curlproc::httpPost(url, body, headers, 
				connectTimeoutSecs, curlproc::DEFAULT_USER_AGENT, customCurlPath);
		}, customRetryCondition);
	}
private:
	const function<string()> _customCurlPathGetter;

	string executeWithRetry(int numRetries, const function<string()>& action, 
		const function<bool(const string&)>& customRetryCondition) const
	{
		string output;
		int retries = 0;

		do {
			output = action();
			if (customRetryCondition(output)) output = "";
		} while (++retries <= numRetries && output.empty());

		return output;
	}
};
