#pragma once

#include "Libraries/inihandler.h"
#include "logging/LoggerBase.h"
#include <string>
using namespace std;

struct ExtensionConfig {
	const bool disabled;
	const string scriptPath;
	const string logDirPath;
	const Logger::Level logLevel;
	const bool appendErrMsg;
	const bool activeThreadOnly;
	const int skipConsoleAndClipboard;
	const bool reloadOnScriptModified;
	const int pipPackageInstallMode; // 0: Skip if installed; 1: Update if installed; 2: Reinstall if installed
	const string pipRequirementsTxtPath;
	const string scriptCustomVars;
	const string scriptCustomVarsDelim;
	const string customPythonPath;

	ExtensionConfig(bool disabled_, const string& scriptPath_, const string& logDirPath_, 
		Logger::Level logLevel_, bool appendErrMsg_, bool activeThreadOnly_, int skipConsoleAndClipboard_,
		bool reloadOnScriptModified_, int pipPackageInstallMode_, const string& pipRequirementsTxtPath_, 
		const string& scriptCustomVars_, const string& scriptCustomVarsDelim_, const string& customPythonPath_)
		: disabled(disabled_), scriptPath(scriptPath_), logDirPath(logDirPath_), logLevel(logLevel_), 
			appendErrMsg(appendErrMsg_), activeThreadOnly(activeThreadOnly_), 
			skipConsoleAndClipboard(skipConsoleAndClipboard_), reloadOnScriptModified(reloadOnScriptModified_), 
			pipPackageInstallMode(pipPackageInstallMode_), pipRequirementsTxtPath(pipRequirementsTxtPath_), 
			scriptCustomVars(scriptCustomVars_), scriptCustomVarsDelim(scriptCustomVarsDelim_),
			customPythonPath(customPythonPath_) { }
};

const ExtensionConfig DefaultConfig = ExtensionConfig(
	false, "", "python-interpretter\\logs\\", Logger::Info, true, false, 1, true, 0, "", "", "||", ""
);


class ConfigRetriever {
public:
	virtual ~ConfigRetriever() { }
	virtual bool configSectionExists() = 0;
	virtual ExtensionConfig getConfig(bool saveDefaultConfigIfNotExist = true) = 0;
	virtual void saveConfig(const ExtensionConfig& config, bool overrideIfExists) = 0;
};


class IniConfigRetriever : public ConfigRetriever {
public:
	IniConfigRetriever(const string& iniFileName, const wstring& iniSectionName)
		: _iniFileName(iniFileName), _iniSectionName(iniSectionName), _iniHandler(iniFileName) { }

	bool configSectionExists();
	ExtensionConfig getConfig(bool saveDefaultConfigIfNotExist = true);
	void saveConfig(const ExtensionConfig& config, bool overrideIfExists);
private:
	IniFileHandler _iniHandler;
	const string _iniFileName;
	const wstring _iniSectionName;

	bool configKeyExists(IniContents& ini, const wstring& key);
	bool setValue(IniContents& ini, const wstring& key, const wstring& value, bool overrideIfExists);
	bool setValue(IniContents& ini, const wstring& key, const string& value, bool overrideIfExists);
	bool setValue(IniContents& ini, const wstring& key, int value, bool overrideIfExists);
	bool setValue(IniContents& ini, const wstring& key, Logger::Level value, bool overrideIfExists);
	string logLevelToStr(Logger::Level logLevel);
	void setDefaultConfig(bool overrideIfExists);

	wstring getValOrDef(IniContents& ini, const wstring& key, wstring defaultValue);
	string getValOrDef(IniContents& ini, const wstring& key, string defaultValue);
	int getValOrDef(IniContents& ini, const wstring& key, int defaultValue);
	Logger::Level getValOrDef(IniContents& ini, const wstring& key, Logger::Level defaultValue);
	Logger::Level strToLogLevel(const string& logLevelStr, Logger::Level defaultValue);
	string unenclose(string str, const char encloseCh);
};
