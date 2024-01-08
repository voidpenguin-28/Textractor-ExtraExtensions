#pragma once

#include "Libraries/curlproc.h"
#include <functional>
#include <string>
#include <vector>
using namespace std;

class HttpClient {
public:
	~HttpClient() { }
	virtual string httpGet(const string& url, const vector<string>& headers = vector<string>()) const = 0;
	virtual string httpPost(const string& url, const string& body, const vector<string>& headers = vector<string>()) const = 0;
};


class CurlProcHttpClient : public HttpClient {
public:
	CurlProcHttpClient(const function<string()> customCurlPathGetter) 
		: _customCurlPathGetter(customCurlPathGetter) { }
	CurlProcHttpClient(const string& customCurlPath) 
		: CurlProcHttpClient([customCurlPath]() { return customCurlPath; }) { }

	string httpGet(const string& url, const vector<string>& headers = vector<string>()) const override {
		string customCurlPath = _customCurlPathGetter();
		return curlproc::httpGet(url, headers, curlproc::DEFAULT_USER_AGENT, customCurlPath);
	}

	string httpPost(const string& url, const string& body, const vector<string>& headers = vector<string>()) const override {
		string customCurlPath = _customCurlPathGetter();
		return curlproc::httpPost(url, body, headers, curlproc::DEFAULT_USER_AGENT, customCurlPath);
	}
private:
	const function<string()> _customCurlPathGetter;
};
