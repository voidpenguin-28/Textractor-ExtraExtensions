
#pragma once
#include <string>
#include <vector>
using namespace std;

class KeyValSplitter {
public:
	virtual ~KeyValSplitter() { }
	virtual vector<pair<string, string>> splitToKeyVals(
		const string& vars, const string varsDelim, const string& keyValDelim = "=") = 0;
};


class DefaultKeyValSplitter : public KeyValSplitter {
public:
	virtual vector<pair<string, string>> splitToKeyVals(
		const string& vars, const string varsDelim, const string& keyValDelim = "=") override
	{
		vector<pair<string, string>> keyVals{};
		string keyVal, key, val;

		const size_t delimLen = varsDelim.length();
		size_t currOffset = 0, prevOffset = 0;

		while (currOffset < vars.length()) {
			prevOffset = currOffset;
			currOffset = vars.find(varsDelim, prevOffset);
			if (currOffset == string::npos) currOffset = vars.length();

			keyVal = vars.substr(prevOffset, currOffset - prevOffset);
			keyVals.push_back(splitToPair(keyVal, keyValDelim));
			currOffset += delimLen;
		}

		return keyVals;
	}

private:
	pair<string, string> splitToPair(const string& str, const string& delim) {
		size_t delimIndex = str.find(delim);
		if (delimIndex == string::npos) delimIndex = str.length();

		string key = str.substr(0, delimIndex);
		string val = delimIndex < str.length() ? str.substr(delimIndex + delim.length()) : "";
		return pair<string, string>(key, val);
	}
};
