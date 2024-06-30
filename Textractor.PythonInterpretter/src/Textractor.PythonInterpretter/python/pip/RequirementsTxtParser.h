
#pragma once
#include <fstream>
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

		while (getline(f, line)) {
			packageName = formatPackageName(packageName);
			if (!isValidPackageName(packageName)) continue;
			packages.push_back(packageName);
		}

		return packages;
	}
private:
	unordered_map<string, string> _customPackageToModuleMap = {
		{ "pypiwin32", "win32file" }
	};

	string formatPackageName(string packageName) {
		packageName = trimSpace(packageName);
		return WinApiHelper::replace(packageName, "-", "_");
	}

	string trimSpace(string str) {
		while (!str.empty() && str[0] == ' ')
			str = str.substr(1);
		while (!str.empty() && str.back() == ' ')
			str = str.substr(0, str.length() - 1);

		return str;
	}

	bool isValidPackageName(const string& packageName) {
		if (packageName.empty()) return false;
		if (!isalpha(packageName[0]) && packageName[0] != '_') return false;

		for (const char ch : packageName) {
			if (!isalnum(ch) && ch != '_') return false;
		}

		return true;
	}
};
