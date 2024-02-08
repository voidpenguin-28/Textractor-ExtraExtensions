#pragma once

#include <string>
#include <vector>
using namespace std;

namespace curlproc {
	const string DEFAULT_USER_AGENT = "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:109.0) Gecko/20100101 Firefox/119.0";

	string httpGet(const string& url, const vector<string>& headers = vector<string>(), int connectTimeoutSecs = 10, const string& userAgent = DEFAULT_USER_AGENT, const string& customCurlPath = "");
	string httpPost(const string& url, const string& body, const vector<string>& headers = vector<string>(), int connectTimeoutSecs = 10, const string& userAgent = DEFAULT_USER_AGENT, const string& customCurlPath = "");
}
