#pragma once

#include "Libraries/inihandler.h"
#include "Libraries/FileTracker.h"
#include "Libraries/Locker.h"
#include "logging/LoggerBase.h"
#include <string>
using namespace std;

struct ExtensionConfig {
	enum ConsoleClipboardMode { SkipNone = 0, SkipAll, SkipConsole, SkipClipboard };

	bool disabled;
	string scriptPath;
	string logDirPath;
	Logger::Level logLevel;
	bool appendErrMsg;
	bool activeThreadOnly;
	ConsoleClipboardMode skipConsoleAndClipboard;
	bool reloadOnScriptModified;
	bool forceScriptReload;
	int pipPackageInstallMode; // 0: Skip if installed; 1: Update if installed; 2: Reinstall if installed
	string pipRequirementsTxtPath;
	int showLogConsole;
	string scriptCustomVars;
	string scriptCustomVarsDelim;
	string customPythonPath;

	ExtensionConfig(bool disabled_, const string& scriptPath_, const string& logDirPath_, 
		Logger::Level logLevel_, bool appendErrMsg_, bool activeThreadOnly_, 
		ConsoleClipboardMode skipConsoleAndClipboard_, bool reloadOnScriptModified_, bool forceScriptReload_, 
		int pipPackageInstallMode_, const string& pipRequirementsTxtPath_, const int showLogConsole_, 
		const string& scriptCustomVars_, const string& scriptCustomVarsDelim_, const string& customPythonPath_)
		: disabled(disabled_), scriptPath(scriptPath_), logDirPath(logDirPath_), logLevel(logLevel_), 
			appendErrMsg(appendErrMsg_), activeThreadOnly(activeThreadOnly_), 
			skipConsoleAndClipboard(skipConsoleAndClipboard_), reloadOnScriptModified(reloadOnScriptModified_), 
			forceScriptReload(forceScriptReload_), pipPackageInstallMode(pipPackageInstallMode_), 
			pipRequirementsTxtPath(pipRequirementsTxtPath_), showLogConsole(showLogConsole_), 
			scriptCustomVars(scriptCustomVars_), scriptCustomVarsDelim(scriptCustomVarsDelim_), 
			customPythonPath(customPythonPath_) { }
};

static const ExtensionConfig DefaultConfig = ExtensionConfig(
	false, "", "python-interpretter\\logs\\", Logger::Info, true, false, 
	ExtensionConfig::ConsoleClipboardMode::SkipAll, true, false, 0, "", 0, "", "||", ""
);


class ConfigRetriever {
public:
	virtual ~ConfigRetriever() { }
	virtual ExtensionConfig getConfig(bool saveDefaultConfigIfNotExist = true) = 0;
	virtual void saveConfig(const ExtensionConfig& config, bool overrideIfExists) = 0;
};


class IniConfigRetriever : public ConfigRetriever {
public:
	IniConfigRetriever(const string& iniFileName, const wstring& iniSectionName)
		: _iniFileName(iniFileName), _iniSectionName(iniSectionName), _iniHandler(iniFileName) { }

	bool configSectionExists() const;
	ExtensionConfig getConfig(bool saveDefaultConfigIfNotExist = true);
	void saveConfig(const ExtensionConfig& config, bool overrideIfExists);
private:
	IniFileHandler _iniHandler;
	const string _iniFileName;
	const wstring _iniSectionName;

	bool configKeyExists(IniContents& ini, const wstring& key) const;
	bool setValue(IniContents& ini, const wstring& key, const wstring& value, bool overrideIfExists);
	bool setValue(IniContents& ini, const wstring& key, const string& value, bool overrideIfExists);
	bool setValue(IniContents& ini, const wstring& key, int value, bool overrideIfExists);
	bool setValue(IniContents& ini, const wstring& key, Logger::Level value, bool overrideIfExists);
	string logLevelToStr(Logger::Level logLevel) const;
	void setDefaultConfig(bool overrideIfExists);

	wstring getValOrDef(IniContents& ini, const wstring& key, wstring defaultValue) const;
	string getValOrDef(IniContents& ini, const wstring& key, string defaultValue) const;
	int getValOrDef(IniContents& ini, const wstring& key, int defaultValue) const;
	template<typename T>
	T getValOrDef(IniContents& ini, const wstring& key, int defaultValue) const;
	Logger::Level getLevelValOrDef(IniContents& ini, const wstring& key, Logger::Level defaultValue) const;
	Logger::Level strToLogLevel(const string& logLevelStr, Logger::Level defaultValue) const;
	string unenclose(string str, const char encloseCh) const;
};


// Keeps a config copy in memory, which is only reloaded when the provided config file is modified.
class FileWatchMemCacheConfigRetriever : public ConfigRetriever {
public:
	FileWatchMemCacheConfigRetriever(ConfigRetriever& mainRetriever, 
		FileTracker& fileTracker, const string& configFilePath) : _mainRetriever(mainRetriever), 
		_fileTracker(fileTracker), _configFilePath(configFilePath), _currConfig(mainRetriever.getConfig(false))
	{
		_lastModifiedTime = getLastModifiedTime();
	}

	ExtensionConfig getConfig(bool saveDefaultConfigIfNotExist = true) override {
		if (saveDefaultConfigIfNotExist || configModified()) updateLastModTimeAndConfig();
		return _currConfig;
	}

	void saveConfig(const ExtensionConfig& config, bool overrideIfExists) override {
		_mainRetriever.saveConfig(config, overrideIfExists);
		updateLastModTimeAndConfig();
	}
private:
	BasicLocker _updateLocker;
	int64_t _lastModifiedTime;
	ExtensionConfig _currConfig;
	ConfigRetriever& _mainRetriever;
	FileTracker& _fileTracker;
	const string _configFilePath;

	const function<void()> _updateLastModTimeAndConfig = [this]() {
		_lastModifiedTime = getLastModifiedTime();
		_currConfig = _mainRetriever.getConfig(true);
	};

	bool configModified() const {
		int64_t currLastModifiedTime = getLastModifiedTime();
		return currLastModifiedTime != _lastModifiedTime;
	}

	void updateLastModTimeAndConfig() {
		_updateLocker.lock(_updateLastModTimeAndConfig);
	}

	int64_t getLastModifiedTime() const {
		return _fileTracker.getDateLastModifiedEpochs(_configFilePath);
	}
};
