#pragma once

#include "_Libraries/inihandler.h"
#include "_Libraries/FileTracker.h"
#include "_Libraries/Locker.h"
#include <string>
#include <unordered_set>
using namespace std;


struct ExtensionConfig {
	enum ConsoleClipboardMode { SkipNone = 0, SkipAll, SkipConsole, SkipClipboard };
	enum FilterMode { Disabled = 0, Blacklist, Whitelist };

	bool disabled;
	wstring logFilePathTemplate; // {0}: process name; {1}: thread key; {2}: thread name
	bool activeThreadOnly;
	ConsoleClipboardMode skipConsoleAndClipboard;
	wstring msgTemplate; // {0}: sentence; {1}: process name; {2}: thread key; {3}: thread name; {4}: current date time
	bool onlyThreadNameAsKey;
	FilterMode threadKeyFilterMode;
	wstring threadKeyFilterList;
	wstring threadKeyFilterListDelim;

	ExtensionConfig(const bool disabled_, const wstring& logFilePathTemplate_, const bool activeThreadOnly_, 
		const ConsoleClipboardMode skipConsoleAndClipboard_, const wstring& msgTemplate_, 
		const bool onlyThreadNameAsKey_, const FilterMode threadKeyFilterMode_, 
		const wstring& threadKeyFilterList_, const wstring& threadKeyFilterListDelim_)
		: disabled(disabled_), logFilePathTemplate(logFilePathTemplate_), activeThreadOnly(activeThreadOnly_), 
			skipConsoleAndClipboard(skipConsoleAndClipboard_), msgTemplate(msgTemplate_),
			onlyThreadNameAsKey(onlyThreadNameAsKey_), threadKeyFilterMode(threadKeyFilterMode_), 
			threadKeyFilterList(threadKeyFilterList_), threadKeyFilterListDelim(threadKeyFilterListDelim_) { }
};

static const ExtensionConfig DefaultConfig = ExtensionConfig(
	false, L"logs\\{0}\\{1}-log.txt", true, 
	ExtensionConfig::ConsoleClipboardMode::SkipAll, 
	L"[{4}][{1}][{2}] {0}", false, ExtensionConfig::FilterMode::Disabled, L"", L"|"
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
