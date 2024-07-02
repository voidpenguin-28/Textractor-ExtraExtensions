
#pragma once
#include "Regex.h"
#include <re2/re2.h>


class RE2Regex : public Regex {
public:
	RE2Regex(const string& pattern) : Regex(pattern, "\\"), _replaceRegex(pattern),
		_regex(formatPattern(pattern)), _numGroups(_regex.NumberOfCapturingGroups()) { }

	bool isMatch(const string& str) override {
		return re2::RE2::FullMatch(str, _regex);
	}

	string findMatch(const string& str) override {
		string match;
		return re2::RE2::PartialMatch(str, _regex, &match) ? match : "";
	}

	vector<string> findMatchCaptures(const string& str) override {
		CaptureArgs cg(_numGroups);
		return re2::RE2::PartialMatchN(str, _regex, &(cg.args[0]), _numGroups) ? convert(cg.ws) : vector<string>{};
	}

	vector<string> findMatches(const string& str) override {
		re2::StringPiece strV(str);
		vector<string> matches{};
		string match;

		while (re2::RE2::FindAndConsume(&strV, _regex, &match)) {
			matches.push_back(match);
		}

		return matches;
	}

	vector<vector<string>> findMatchesCaptures(const string& str) override {
		vector<vector<string>> totalCaptures{};
		vector<string> captures;
		re2::StringPiece strV(str);
		CaptureArgs cg(_numGroups);

		while (re2::RE2::FindAndConsumeN(&strV, _regex, &(cg.args[0]), _numGroups)) {
			captures = convert(cg.ws);
			totalCaptures.push_back(captures);
			cg = CaptureArgs(_numGroups);
		}

		return totalCaptures;
	}

	string replace(string str, const string& replacement) override {
		re2::RE2::GlobalReplace(&str, _replaceRegex, replacement);
		return str;
	}

	vector<string> split(const string& str) override {
		vector<string> result{};
		size_t pos = 0, last_pos = 0;
		RE2 re(pattern);

		re2::StringPiece str_piece(str);
		string piece;

		while (RE2::FindAndConsume(&str_piece, re, &piece)) {
			pos = str_piece.data() - str.data();
			result.push_back(str.substr(last_pos, pos - last_pos - piece.length()));
			last_pos = pos;
		}

		result.push_back(str.substr(last_pos));
		return result;
	}
private:
	static constexpr char _parenthStart = '(';
	static constexpr char _parenthEnd = ')';
	const string _splitDelim = "||~`~||";
	const re2::RE2 _regex;
	const re2::RE2 _replaceRegex;
	const int _numGroups;

	static string formatPattern(string pattern) {
		if (!isWrappedInParenthesis(pattern))
			pattern = wrapInParenthesis(pattern);

		return pattern;
	}

	static bool isWrappedInParenthesis(const string& s) {
		if (s.length() < 2) return false;
		if (s[0] != _parenthStart || s.back() != _parenthEnd) return false;

		int outerCount = 0;
		int counter = 0;

		for (const char c : s) {
			if (c == _parenthStart) counter++;
			else if (c == _parenthEnd) counter--;
			else continue;

			if (counter == 0) outerCount++;
		}

		return outerCount == 1;
	}

	static string wrapInParenthesis(const string& s) {
		return _parenthStart + s + _parenthEnd;
	}

	vector<string> convert(const vector<re2::StringPiece> list) {
		return vector<string>(list.begin(), list.end());
	}

	struct CaptureArgs {
		vector<re2::RE2::Arg> argv;
		vector<re2::RE2::Arg*> args;
		vector<re2::StringPiece> ws;

		CaptureArgs(const int numGroups) : argv(vector<re2::RE2::Arg>(numGroups)),
			args(vector<re2::RE2::Arg*>(numGroups)), ws(vector<re2::StringPiece>(numGroups))
		{
			for (int i = 0; i < numGroups; ++i) {
				args[i] = &argv[i];
				argv[i] = &ws[i];
			}
		}
	};
};
