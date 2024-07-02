
#pragma once
#include <codecvt>
#include <string>
#include <vector>
using namespace std;

class Regex {
public:
	Regex(const string& pattern, const string& replaceSequence) :
		pattern(pattern), replaceSequence(replaceSequence) { }

	virtual ~Regex() { }
	const string pattern;
	const string replaceSequence;

	virtual bool isMatch(const string& str) = 0;
	virtual string findMatch(const string& str) = 0;
	virtual vector<string> findMatchCaptures(const string& str) = 0;
	virtual vector<string> findMatches(const string& str) = 0;
	virtual vector<vector<string>> findMatchesCaptures(const string& str) = 0;
	virtual string replace(string str, const string& replacement) = 0;
	virtual vector<string> split(const string& str) = 0;
};

class WRegex {
public:
	WRegex(const wstring& pattern, const wstring& replaceSequence) :
		pattern(pattern), replaceSequence(replaceSequence) { }

	virtual ~WRegex() { }
	const wstring pattern;
	const wstring replaceSequence;

	virtual bool isMatch(const wstring& str) = 0;
	virtual wstring findMatch(const wstring& str) = 0;
	virtual vector<wstring> findMatchCaptures(const wstring& str) = 0;
	virtual vector<wstring> findMatches(const wstring& str) = 0;
	virtual vector<vector<wstring>> findMatchesCaptures(const wstring& str) = 0;
	virtual wstring replace(wstring str, const wstring& replacement) = 0;
	virtual vector<wstring> split(const wstring& str) = 0;
};


class AdapterWRegex : public WRegex {
public:
	AdapterWRegex(Regex& baseRegex) : WRegex(convertToW(baseRegex.pattern), 
		convertToW(baseRegex.replaceSequence)), _baseRegex(baseRegex) { }

	bool isMatch(const wstring& str) override {
		return _baseRegex.isMatch(convertFromW(str));
	}

	wstring findMatch(const wstring& str) override {
		string match = _baseRegex.findMatch(convertFromW(str));
		return convertToW(match);
	}

	vector<wstring> findMatchCaptures(const wstring& str) override {
		vector<string> captures = _baseRegex.findMatchCaptures(convertFromW(str));
		return convertToW(captures);
	}

	vector<wstring> findMatches(const wstring& str) override {
		vector<string> matches = _baseRegex.findMatches(convertFromW(str));
		return convertToW(matches);
	}

	vector<vector<wstring>> findMatchesCaptures(const wstring& str) override {
		vector<vector<string>> matches = _baseRegex.findMatchesCaptures(convertFromW(str));
		vector<vector<wstring>> wMatches{};

		for (const vector<string>& captures : matches) {
			wMatches.push_back(convertToW(captures));
		}
		
		return wMatches;
	}

	wstring replace(wstring str, const wstring& replacement) override {
		string newStr = _baseRegex.replace(convertFromW(str), convertFromW(replacement));
		return convertToW(newStr);
	}

	vector<wstring> split(const wstring& str) override {
		vector<string> splits = _baseRegex.split(convertFromW(str));
		return convertToW(splits);
	}
private:
	Regex& _baseRegex;

	static wstring convertToW(const string& str) {
		return wstring_convert<codecvt_utf8_utf16<wchar_t>>().from_bytes(str);
	}

	static string convertFromW(const wstring& str) {
		return wstring_convert<codecvt_utf8_utf16<wchar_t>>().to_bytes(str);
	}

	vector<wstring> convertToW(const vector<string>& v) {
		vector<wstring> wv{};

		for (const string& str : v) {
			wv.push_back(convertToW(str));
		}

		return wv;
	}
};
