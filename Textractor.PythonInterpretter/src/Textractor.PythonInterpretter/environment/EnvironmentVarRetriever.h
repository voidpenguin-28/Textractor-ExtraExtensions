
#pragma once
#include <cstdlib>
#include <string>
using namespace std;

class EnvironmentVarRetriever {
public:
	virtual  ~EnvironmentVarRetriever() { }
	virtual string getTempDirVarName() const = 0;
	virtual string getEnvVar(const string& varName) = 0;
};


class WinStdLibEnvironmentVarRetriever : public EnvironmentVarRetriever {
public:
	string getTempDirVarName() const override {
		static const string varName = "TEMP";
		return varName;
	}

	string getEnvVar(const string& varName) override {
		char* buf = nullptr;
		size_t sz = 0;

		if (_dupenv_s(&buf, &sz, varName.c_str()) != 0 || buf == nullptr) return "";

		string val = buf;
		free(buf);
		return val;
	}
};
