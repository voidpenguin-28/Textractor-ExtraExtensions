#pragma once

#include <string>
#include <vector>
using namespace std;

namespace curlproc {
	const string DEFAULT_USER_AGENT = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/147.0.0.0 Safari/537.36";

	string httpGet(const string& url, const vector<string>& headers = vector<string>(), const string& userAgent = DEFAULT_USER_AGENT, const string& customCurlPath = "");
	string httpPost(const string& url, const string& body, const vector<string>& headers = vector<string>(), const string& userAgent = DEFAULT_USER_AGENT, const string& customCurlPath = "");
}
