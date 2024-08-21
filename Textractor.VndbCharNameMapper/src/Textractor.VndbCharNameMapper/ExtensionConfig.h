#pragma once

#include "Libraries/inihandler.h"
#include "Libraries/FileTracker.h"
#include "Libraries/Locker.h"
#include <string>
using namespace std;


struct ExtensionConfig {
	enum MappingMode { None, Name, NameAndGender };
	enum ConsoleClipboardMode { SkipNone = 0, SkipAll, SkipConsole, SkipClipboard };

	bool disabled;
	string urlTemplate;
	wstring vnIds;
	wstring vnIdDelim;
	MappingMode mappingMode;
	int minNameCharSize;
	bool activeThreadOnly;
	ConsoleClipboardMode skipConsoleAndClipboard;
	bool reloadCacheOnLaunch;
	string customCurlPath;

	ExtensionConfig(bool disabled_, string urlTemplate_, wstring vnIds_, wstring vnIdDelim_, 
		MappingMode mappingMode_, int minNameCharSize_, bool activeThreadOnly_,
		ConsoleClipboardMode skipConsoleAndClipboard_, bool reloadCacheOnLaunch_, string customCurlPath_)
		: disabled(disabled_), urlTemplate(urlTemplate_), vnIds(vnIds_), vnIdDelim(vnIdDelim_),
			mappingMode(mappingMode_), minNameCharSize(minNameCharSize_), 
			activeThreadOnly(activeThreadOnly_),skipConsoleAndClipboard(skipConsoleAndClipboard_), 
			reloadCacheOnLaunch(reloadCacheOnLaunch_), customCurlPath(customCurlPath_) { }
};

using MappingMode = ExtensionConfig::MappingMode;

static const ExtensionConfig DefaultConfig = ExtensionConfig(
	false, "https://vndb.org/{0}/chars", L"", L"|", MappingMode::Name, 2, true, 
	ExtensionConfig::ConsoleClipboardMode::SkipAll, false, ""
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
	ExtensionConfig getConfig(bool saveDefaultConfigIfNotExist = true) override;
	void saveConfig(const ExtensionConfig& config, bool overrideIfExists) override;
private:
	const IniFileHandler _iniHandler;
	const string _iniFileName;
	const wstring _iniSectionName;

	bool configKeyExists(IniContents& ini, const wstring& key) const;
	bool setValue(IniContents& ini, const wstring& key, const wstring& value, bool overrideIfExists);
	bool setValue(IniContents& ini, const wstring& key, const string& value, bool overrideIfExists);
	bool setValue(IniContents& ini, const wstring& key, int value, bool overrideIfExists);
	void setDefaultConfig(bool overrideIfExists);
	wstring getValOrDef(IniContents& ini, const wstring& key, wstring defaultValue) const;
	string getValOrDef(IniContents& ini, const wstring& key, string defaultValue) const;
	int getValOrDef(IniContents& ini, const wstring& key, int defaultValue) const;
	template<typename T>
	T getValOrDef(IniContents& ini, const wstring& key, int defaultValue) const;
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
