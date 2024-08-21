#pragma once

#include "../_Libraries/curlproc.h"
#include "../_Libraries/Locker.h"
#include "../_Libraries/winmsg.h"
#include <curl/curl.h>
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
		const function<bool(const string&)>& customRetryCondition = {}) = 0;
	virtual string httpPost(const string& url, const string& body, const vector<string>& headers = vector<string>(),
		int connectTimeoutSecs = DEFAULT_CONNECT_TIMEOUT_SECS, int numRetries = DEFAULT_NUM_RETRIES,
		const function<bool(const string&)>& customRetryCondition = {}) = 0;
};

class BasicStubHttpClient : public HttpClient {
public:
	BasicStubHttpClient(const string& getResponse, const string& postResponse)
		: _getResponse(getResponse), _postResponse(postResponse) { }
	BasicStubHttpClient(const string& response) : _getResponse(response), _postResponse(response) { }

	string httpGet(const string& url, const vector<string>& headers = vector<string>(),
		int connectTimeoutSecs = DEFAULT_CONNECT_TIMEOUT_SECS, int numRetries = DEFAULT_NUM_RETRIES,
		const function<bool(const string&)>& customRetryCondition = {})
	{
		return _getResponse;
	}

	string httpPost(const string& url, const string& body, const vector<string>& headers = vector<string>(),
		int connectTimeoutSecs = DEFAULT_CONNECT_TIMEOUT_SECS, int numRetries = DEFAULT_NUM_RETRIES,
		const function<bool(const string&)>& customRetryCondition = {})
	{
		return _postResponse;
	}
private:
	const string _getResponse;
	const string _postResponse;
};


class PerThreadHttpClient : public HttpClient {
public:
	PerThreadHttpClient(const function<HttpClient*()>& clientCreator) : _clientCreator(clientCreator) { }

	string httpGet(const string& url, const vector<string>& headers = vector<string>(),
		int connectTimeoutSecs = DEFAULT_CONNECT_TIMEOUT_SECS, int numRetries = DEFAULT_NUM_RETRIES,
		const function<bool(const string&)>& customRetryCondition = {})
	{
		HttpClient& client = getOrCreateClient();
		return client.httpGet(url, headers, connectTimeoutSecs, numRetries, customRetryCondition);
	}

	string httpPost(const string& url, const string& body, const vector<string>& headers = vector<string>(),
		int connectTimeoutSecs = DEFAULT_CONNECT_TIMEOUT_SECS, int numRetries = DEFAULT_NUM_RETRIES,
		const function<bool(const string&)>& customRetryCondition = {})
	{
		HttpClient& client = getOrCreateClient();
		return client.httpPost(url, body, headers, connectTimeoutSecs, numRetries, customRetryCondition);
	}
private:
	mutable BasicLocker _locker;
	unordered_map<thread::id, unique_ptr<HttpClient>> _threadClientMap;
	function<HttpClient*()> _clientCreator;

	HttpClient& getOrCreateClient() {
		HttpClient* client = nullptr;

		_locker.lock([this, &client]() {
			thread::id threadId = getThreadId();

			if (_threadClientMap.find(threadId) == _threadClientMap.end())
				_threadClientMap[threadId] = unique_ptr<HttpClient>(_clientCreator());

			client = _threadClientMap[threadId].get();
		});

		return *client;
	}

	thread::id getThreadId() {
		return this_thread::get_id();
	}
};


class ActionRetry {
public:
	virtual ~ActionRetry() { }
	virtual string executeWithRetry(int numRetries,
		const function<string()>& action, const function<bool(const string&)>& retryCondition) = 0;
};

class DefaultActionRetry : public ActionRetry {
public:
	string executeWithRetry(int numRetries, const function<string()>& action,
		const function<bool(const string&)>& retryCondition) override
	{
		string output;
		int retries = 0;

		do {
			output = action();
			if (retryCondition(output)) output = "";
		} while (++retries <= numRetries && output.empty());

		return output;
	}
};

class LibCurlHttpClient : public HttpClient {
public:
	LibCurlHttpClient() {
		_instanceLocker.lock([this]() {
			_curl = createCurl();
			localInitCurl();
			_instances++;
		});
	}

	~LibCurlHttpClient() {
		_instanceLocker.lock([this]() {
			_instances--;
			deallocateCurl();
		});
	}

	string httpGet(const string& url, const vector<string>& headers = vector<string>(),
		int connectTimeoutSecs = DEFAULT_CONNECT_TIMEOUT_SECS, int numRetries = DEFAULT_NUM_RETRIES,
		const function<bool(const string&)>& customRetryCondition = {}) override
	{
		return httpRequest(false, url, "", headers, connectTimeoutSecs, numRetries, customRetryCondition);
	}

	string httpPost(const string& url, const string& body, const vector<string>& headers = vector<string>(),
		int connectTimeoutSecs = DEFAULT_CONNECT_TIMEOUT_SECS, int numRetries = DEFAULT_NUM_RETRIES,
		const function<bool(const string&)>& customRetryCondition = {}) override
	{
		return httpRequest(true, url, body, headers, connectTimeoutSecs, numRetries, customRetryCondition);
	}
private:
	static BasicLocker _instanceLocker;
	static int _instances;

	const string _userAgent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:109.0) Gecko/20100101 Firefox/119.0";
	mutable BasicLocker _locker;
	DefaultActionRetry _actionRetry;

	CURL* _curl = nullptr;
	struct curl_slist* _headerList = nullptr;
	string _readBuffer = "";
	bool _currRequestPost = true;
	string _currUrl = "";
	string _currBody = "";
	vector<string> _currHeaders{};
	int _currConnectTimeoutSecs = DEFAULT_CONNECT_TIMEOUT_SECS;

	static void globalInitCurl() {
		if (_instances == 0) curl_global_init(CURL_GLOBAL_ALL);
	}

	static CURL* createCurl() {
		globalInitCurl();
		return curl_easy_init();
	}

	static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
		((std::string*)userp)->append((char*)contents, size * nmemb);
		return size * nmemb;
	}

	void localInitCurl() {
		curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, writeCallback);
		curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &_readBuffer);
		curl_easy_setopt(_curl, CURLOPT_USERAGENT, _userAgent.c_str());
		curl_easy_setopt(_curl, CURLOPT_FOLLOWLOCATION, 1L);
		assignConnectTimeoutSecs(_currConnectTimeoutSecs);
	}

	void resetCurl() {
		curl_easy_reset(_curl);
		_currUrl = "";
		_currBody = "";
		_currHeaders = {};
		_currConnectTimeoutSecs = DEFAULT_CONNECT_TIMEOUT_SECS;
		localInitCurl();
	}

	void deallocateCurl() {
		freeHeaderList();
		curl_easy_cleanup(_curl);
		if (_instances == 0) curl_global_cleanup();
	}

	string httpRequest(bool requestPost, const string& url, const string& body,
		const vector<string>& headers = vector<string>(), int connectTimeoutSecs = DEFAULT_CONNECT_TIMEOUT_SECS,
		int numRetries = DEFAULT_NUM_RETRIES, const function<bool(const string&)>& customRetryCondition = {})
	{
		return _locker.lockS([this, requestPost, &url, &body, &headers, 
			connectTimeoutSecs, numRetries, &customRetryCondition]()
		{ 
			return httpRequestBase(requestPost, url, body, headers,
				connectTimeoutSecs, numRetries, customRetryCondition); 
		});
	}

	string httpRequestBase(bool requestPost, const string& url, const string& body, 
		const vector<string>& headers = vector<string>(), int connectTimeoutSecs = DEFAULT_CONNECT_TIMEOUT_SECS, 
		int numRetries = DEFAULT_NUM_RETRIES, const function<bool(const string&)>& customRetryCondition = {})
	{
		if (requestPost != _currRequestPost) {
			_currRequestPost = requestPost;
			resetCurl();
		}

		if (url != _currUrl) assignUrl(url);
		if (headerListChanged(headers)) assignHeaderList(headers);
		if (connectTimeoutSecs = _currConnectTimeoutSecs) assignConnectTimeoutSecs(connectTimeoutSecs);
		
		_currBody = body;
		if (_currRequestPost) curl_easy_setopt(_curl, CURLOPT_POSTFIELDS, _currBody.c_str());

		return execCurl(numRetries);
	}

	void assignUrl(const string& url) {
		_currUrl = url;
		curl_easy_setopt(_curl, CURLOPT_URL, _currUrl.c_str());
	}

	void assignConnectTimeoutSecs(const int connectTimeoutSecs) {
		_currConnectTimeoutSecs = connectTimeoutSecs;
		curl_easy_setopt(_curl, CURLOPT_TIMEOUT_MS, _currConnectTimeoutSecs * 1000);
	}

	bool headerListChanged(const vector<string>& headers) const {
		if (headers.size() != _currHeaders.size()) return true;

		for (size_t i = 0; i < headers.size(); i++) {
			if (headers[i] != _currHeaders[i]) return true;
		}

		return false;
	}

	void assignHeaderList(const vector<string>& headers) {
		if (_headerList != nullptr) freeHeaderList();
		_currHeaders = headers;

		for (const string& header : _currHeaders) {
			_headerList = curl_slist_append(_headerList, header.c_str());
		}

		curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, _headerList);
	}

	void freeHeaderList() {
		curl_slist_free_all(_headerList);
		_headerList = nullptr;
	}

	string execCurl(int numRetries) {
		return _actionRetry.executeWithRetry(numRetries, [this]() {
			_readBuffer = "";
			CURLcode res = curl_easy_perform(_curl);
			
			return res == CURLE_OK ? _readBuffer : 
				"The server had an error processing your request.\n" + string(curl_easy_strerror(res));
		}, [](const string& o) { return o.empty(); });
	}
};


class CurlProcHttpClient : public HttpClient {
public:
	CurlProcHttpClient(const function<string()> customCurlPathGetter)
		: _customCurlPathGetter(customCurlPathGetter) { }
	CurlProcHttpClient(const string& customCurlPath)
		: CurlProcHttpClient([customCurlPath]() { return customCurlPath; }) { }

	string httpGet(const string& url, const vector<string>& headers = vector<string>(),
		int connectTimeoutSecs = DEFAULT_CONNECT_TIMEOUT_SECS, int numRetries = DEFAULT_NUM_RETRIES,
		const function<bool(const string&)>& customRetryCondition = {}) override
	{
		string customCurlPath = _customCurlPathGetter();

		return _actionRetry.executeWithRetry(numRetries, [&url, &headers, connectTimeoutSecs, &customCurlPath]() {
			return curlproc::httpGet(url, headers,
				connectTimeoutSecs, curlproc::DEFAULT_USER_AGENT, customCurlPath);
		}, customRetryCondition);
	}

	string httpPost(const string& url, const string& body, const vector<string>& headers = vector<string>(),
		int connectTimeoutSecs = DEFAULT_CONNECT_TIMEOUT_SECS, int numRetries = DEFAULT_NUM_RETRIES,
		const function<bool(const string&)>& customRetryCondition = {}) override
	{
		string customCurlPath = _customCurlPathGetter();

		return _actionRetry.executeWithRetry(numRetries, 
			[&url, &body, &headers, connectTimeoutSecs, &customCurlPath]()
		{
			return curlproc::httpPost(url, body, headers, 
				connectTimeoutSecs, curlproc::DEFAULT_USER_AGENT, customCurlPath);
		}, customRetryCondition);
	}
private:
	DefaultActionRetry _actionRetry;
	const function<string()> _customCurlPathGetter;
};
