
#pragma once
#include "../../logging/LoggerBase.h"
#include "../ProcessManager.h"
#include "../WinApiHelper.h"
#include "PipCmdStrBuilder.h"
#include <cstdio>
#include <functional>
#include <string>
#include <vector>
using namespace std;


class PipPackageInstaller {
public:
	virtual ~PipPackageInstaller() { }
	
	virtual bool install(const string& packageName) = 0;
	virtual bool install(const vector<string>& packageNames) = 0;
	virtual bool installFromFile(const string& requirementsTxtPath) = 0;
};


class NoPipPackageInstaller : public PipPackageInstaller {
public:
	NoPipPackageInstaller(bool returnVal = true) : _returnVal(returnVal) { }
	
	bool install(const string& packageName) override {
		return _returnVal;
	}

	bool install(const vector<string>& packageNames) override {
		return _returnVal;
	}

	bool installFromFile(const string& requirementsTxtPath) override {
		return _returnVal;
	}
private:
	const bool _returnVal;
};


class ProcManagerPipPackageInstaller : public PipPackageInstaller {
public:
	ProcManagerPipPackageInstaller(const Logger& logger, 
		ProcessManager& procManager, PipCmdStrBuilder& cmdStrBuilder)
		: _logger(logger), _procManager(procManager), _cmdStrBuilder(cmdStrBuilder) { }

	bool install(const string& packageName) override {
		if (packageName.empty()) return false;
		_logger.logInfo("Installing the following pip package: " + packageName);
		string cmd = _cmdStrBuilder.createInstallCmd(packageName);
		return installFromCommand(cmd);
	}

	bool install(const vector<string>& packageNames) override {
		if (packageNames.empty()) return true;
		_logger.logInfo("Installing the following pip packages: " + WinApiHelper::join(", ", packageNames));
		string cmd = _cmdStrBuilder.createInstallCmd(packageNames);
		return installFromCommand(cmd);
	}

	bool installFromFile(const string& requirementsTxtPath) override {
		_logger.logInfo("Installing the pip packages from the following requirements txt file: " + requirementsTxtPath);
		string cmd = _cmdStrBuilder.createReqsTxtInstallCmd(requirementsTxtPath);
		return installFromCommand(cmd);
	}
private:
	static constexpr DWORD _pipWaitMs = 10 * 60 * 1000; // 10 minutes
	const bool _hidePipWindow = false;
	const Logger& _logger;
	ProcessManager& _procManager;
	PipCmdStrBuilder& _cmdStrBuilder;

	bool installFromCommand(const string& cmd) {
		_logger.logDebug("Installing pip packages with the following command: " + cmd);
		DWORD errCode = _procManager.createProcess(cmd, _hidePipWindow);

		if (WinApiHelper::isErr(errCode)) {
			_logger.logError("Failed to launch 'pip' process. ErrCode: " + to_string(errCode));
			return false;
		}

		if (_procManager.isProcessActive()) {
			if (!_procManager.waitForProcessExit(_pipWaitMs)) {
				_logger.logWarning("Pip process has not finished within timeout period of " + to_string(_pipWaitMs) + "ms. Force closing pip process.");
				_procManager.closeProcess();
				return false;
			}
		}

		DWORD exitStatus;
		if (!_procManager.exitedSuccessfully(exitStatus, errCode)) {
			_logger.logError("Pip process did not exit successfully. An error may have occurred while installing packages. ExitCode: " + to_string(exitStatus));
			return false;
		}

		_procManager.closeProcess();
		_logger.logInfo("Pip packages installed successfully.");
		return true;
	}
};


class POpenPipPackageInstaller : public PipPackageInstaller {
public:
	POpenPipPackageInstaller(const Logger& logger, PipCmdStrBuilder& cmdStrBuilder)
		: _logger(logger), _cmdStrBuilder(cmdStrBuilder) { }

	bool install(const string& packageName) override {
		if (packageName.empty()) return false;
		string cmd = _cmdStrBuilder.createInstallCmd(packageName);
		return installFromCommand(cmd);
	}

	bool install(const vector<string>& packageNames) override {
		if (packageNames.empty()) return true;
		string cmd = _cmdStrBuilder.createInstallCmd(packageNames);
		return installFromCommand(cmd);
	}

	bool installFromFile(const string& requirementsTxtPath) override {
		string cmd = _cmdStrBuilder.createReqsTxtInstallCmd(requirementsTxtPath);
		return installFromCommand(cmd);
	}
private:
	const vector<string> _errTags = { "ERROR:", "Usage:", "was unexpected at this time."};
	const Logger& _logger;
	PipCmdStrBuilder& _cmdStrBuilder;

	bool installFromCommand(const string& cmd) {
		_logger.logInfo("Installing pip packages with the following command: " + cmd);
		string result = runCommand(cmd);

		bool error = hasError(result);
		_logger.log(error ? Logger::Warning : Logger::Debug, result);
		return !error;
	}

	string runCommand(const string& cmd) {
		FILE* f = _popen(cmd.c_str(), "r");
		string result = getFileResult(f);
		
		_pclose(f);
		return result;
	}

	string getFileResult(FILE* f) {
		static constexpr int BUFFER_SIZE = 1024;
		string result;
		char buffer[BUFFER_SIZE]{};

		while (fgets(buffer, sizeof(buffer), f) != nullptr) {
			result += buffer;
		}

		return result;
	}

	bool hasError(const string& pipResult) {
		for (const auto& errTag : _errTags) {
			if (pipResult.find(errTag) != string::npos) return true;
		}

		return false;
	}
};
