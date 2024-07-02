
#pragma once
#include "Regex.h"
#include <regex>


class StdLibRegex : public Regex {
public:
	StdLibRegex(const string& pattern) : Regex(pattern, "$"), _regex(regex(pattern)) { }

	bool isMatch(const string& str) override {
		return regex_match(str, _regex);
	}

	string findMatch(const string& str) override {
		smatch match;
		return regex_search(str, match, _regex) ? match.str() : "";
	}

	vector<string> findMatchCaptures(const string& str) override {
		vector<string> groups{};
		smatch match;
		if (!regex_search(str, match, _regex)) return vector<string>{};

		for (const auto& group : match) {
			groups.push_back(group.str());
		}

		return groups;
	}

	vector<string> findMatches(const string& str) override {
		vector<string> matches{};
		sregex_iterator iter(str.begin(), str.end(), _regex);
		sregex_iterator end;

		while (iter != end) {
			matches.push_back(iter->str());
			++iter;
		}

		return matches;
	}

	vector<vector<string>> findMatchesCaptures(const string& str) override {
		vector<vector<string>> matches{};
		vector<string> captures;
		sregex_iterator iter(str.begin(), str.end(), _regex);
		sregex_iterator end;

		while (iter != end) {
			captures = vector<string>{};

			for (unsigned i = 0; i < iter->size(); ++i) {
				captures.push_back((*iter)[i].str());
			}

			matches.push_back(captures);
			++iter;
		}

		return matches;
	}

	string replace(string str, const string& replacement) override {
		return regex_replace(str, _regex, replacement);
	}

	vector<string> split(const string& str) override {
		sregex_token_iterator it(str.begin(), str.end(), _regex, -1);
		sregex_token_iterator end;
		vector<string> m;

		while (it != end) {
			m.push_back(*it);
			++it;
		}

		return m;
	}
private:
	regex _regex;
};

