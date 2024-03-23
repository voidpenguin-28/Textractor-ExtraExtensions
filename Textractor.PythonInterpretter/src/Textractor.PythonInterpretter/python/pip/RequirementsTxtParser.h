
#pragma once
#include <fstream>
#include <regex>
#include <string>
#include <vector>
using namespace std;


class RequirementsTxtParser {
public:
	virtual ~RequirementsTxtParser() { }
	virtual vector<string> getPackageNames(const string& reqsTxtFilePath) = 0;
};


class RegexRequirementsTxtParser : public RequirementsTxtParser {
public:
	vector<string> getPackageNames(const string& reqsTxtFilePath) override {
		ifstream f(reqsTxtFilePath);
		vector<string> packages{};
		string line, packageName;
		smatch regMatch;

		while (getline(f, line)) {
			if (!regex_match(line, regMatch, _packageNamePattern)) continue;
			packageName = formatPackageName(regMatch[0].str());
			packages.push_back(packageName);
		}

		return packages;
	}
private:
	regex _packageNamePattern = regex("^[-_a-zA-Z]+");
	unordered_map<string, string> _customPackageToModuleMap = {
		{ "pypiwin32", "win32file" }
	};

	string formatPackageName(const string& packageName) {
		return WinApiHelper::replace(packageName, "-", "_");
	}
};
