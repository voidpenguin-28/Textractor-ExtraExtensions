
#pragma once
#include "../../Libraries/strhelper.h"
#include <string>
#include <vector>
using namespace std;


class RequirementsTxtParser {
public:
	virtual ~RequirementsTxtParser() { }
	virtual vector<string> getPackageNames(const string& reqsTxtFilePath) = 0;
};


class DefaultRequirementsTxtParser : public RequirementsTxtParser {
public:
	vector<string> getPackageNames(const string& reqsTxtFilePath) override {
		ifstream f(reqsTxtFilePath);
		vector<string> packages{};
		string line, packageName;

		while (getline(f, line)) {
			packageName = formatPackageName(line);
			if (!isValidPackageName(packageName)) continue;
			packages.push_back(packageName);
		}

		return packages;
	}
private:
	const string SPACE = " ";
	unordered_map<string, string> _customPackageToModuleMap = {
		{ "pypiwin32", "win32file" }
	};
	
	string formatPackageName(string packageName) {
		packageName = StrHelper::trim<char>(packageName, SPACE);
		return StrHelper::replace<char>(packageName, "-", "_");
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
