
#pragma once
#include <string>
#include <unordered_map>
#include <vector>
using namespace std;

class CustomPackageToModuleMapper {
public:
	virtual  ~CustomPackageToModuleMapper() { }
	virtual string mapToModule(const string& packageName) = 0;
	virtual vector<string> mapToModules(const vector<string>& packageNames) = 0;
};


class DefaultCustomPackageToModuleMapper : public CustomPackageToModuleMapper {
public:
	virtual string mapToModule(const string& packageName) override {
		return isCustomMappedPackage(packageName) ? getModuleMapping(packageName) : packageName;
	}

	virtual vector<string> mapToModules(const vector<string>& packageNames) override {
		vector<string> modules{};

		for (const auto& package : packageNames) {
			modules.push_back(mapToModule(package));
		}

		return modules;
	}
private:
	unordered_map<string, string> _customPackageToModuleMap = {
		{ "pypiwin32", "win32file" }
	};

	bool isCustomMappedPackage(const string& packageName) {
		return _customPackageToModuleMap.find(packageName) != _customPackageToModuleMap.end();
	}

	string getModuleMapping(const string& packageName) {
		return _customPackageToModuleMap[packageName];
	}
};
