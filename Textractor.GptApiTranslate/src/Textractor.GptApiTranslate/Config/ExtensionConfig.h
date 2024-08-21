#pragma once

#include "../_Libraries/inihandler.h"
#include "../_Libraries/FileTracker.h"
#include "../_Libraries/Locker.h"
#include "Common.h"
#include <string>
#include <vector>
using namespace std;


struct ExtensionConfig {
	enum ConsoleClipboardMode { SkipNone = 0, SkipAll, SkipConsole, SkipClipboard };
	enum FilterMode { Disabled = 0, Blacklist, Whitelist };
	enum NameMappingMode { None, Name, NameAndGender };

	bool disabled;
	string url;
	string apiKey;
	string model;
	int timeoutSecs;
	int numRetries;
	wstring sysMsgPrefix;
	wstring userMsgPrefix;
	NameMappingMode nameMappingMode;
	bool activeThreadOnly;
	ConsoleClipboardMode skipConsoleAndClipboard;
	bool useHistoryForNonActiveThreads;
	int msgHistoryCount;
	int historySoftCharLimit;
	int msgCharLimit;
	bool skipAsciiText;
	bool skipIfZeroWidthSpace;
	bool showErrMsg;
	string customRequestTemplate;
	string customResponseMsgRegex;
	string customErrorMsgRegex;
	string customHttpHeaders;
	FilterMode threadKeyFilterMode;
	wstring threadKeyFilterList;
	wstring threadKeyFilterListDelim;
	bool debugMode;

	ExtensionConfig(bool disabled_, string url_, string apiKey_, string model_, 
		int timeoutSecs_, int numRetries_, wstring sysMsgPrefix_, wstring userMsgPrefix_, 
		NameMappingMode nameMappingMode_, bool activeThreadOnly_, ConsoleClipboardMode skipConsoleAndClipboard_,
		bool useHistoryForNonActiveThreads_, int msgHistoryCount_, int historySoftCharLimit_, 
		int msgCharLimit_, bool skipAsciiText_, bool skipIfZeroWidthSpace_, bool showErrMsg_, 
		const string& customRequestTemplate_, const string& customResponseMsgRegex_, 
		const string& customErrorMsgRegex_, const string& customHttpHeaders_,
		const FilterMode threadKeyFilterMode_, const wstring& threadKeyFilterList_, 
		const wstring& threadKeyFilterListDelim_, bool debugMode_)
		: disabled(disabled_), url(url_), apiKey(apiKey_), model(model_), 
			timeoutSecs(timeoutSecs_), numRetries(numRetries_), sysMsgPrefix(sysMsgPrefix_), 
			userMsgPrefix(userMsgPrefix_), nameMappingMode(nameMappingMode_),
			activeThreadOnly(activeThreadOnly_), skipConsoleAndClipboard(skipConsoleAndClipboard_),
			useHistoryForNonActiveThreads(useHistoryForNonActiveThreads_), msgHistoryCount(msgHistoryCount_),
			historySoftCharLimit(historySoftCharLimit_), msgCharLimit(msgCharLimit_), 
			skipAsciiText(skipAsciiText_), skipIfZeroWidthSpace(skipIfZeroWidthSpace_),
			showErrMsg(showErrMsg_), customRequestTemplate(customRequestTemplate_), 
			customResponseMsgRegex(customResponseMsgRegex_), customErrorMsgRegex(customErrorMsgRegex_),
			customHttpHeaders(customHttpHeaders_), threadKeyFilterMode(threadKeyFilterMode_),
			threadKeyFilterList(threadKeyFilterList_), threadKeyFilterListDelim(threadKeyFilterListDelim_), 
			debugMode(debugMode_) { }
};

static const ExtensionConfig DefaultConfig = ExtensionConfig(
	false, "https://api.openai.com/v1/chat/completions", 
	"", GPT_MODEL4_O, 10, 2,
	L"Translate novel script to natural fluent EN. Preserve numbering. Use all JP input lines as context (previous lines). However, only return the translation for the line that starts with '99:'.",
	L"", ExtensionConfig::NameMappingMode::None, true, 
	ExtensionConfig::ConsoleClipboardMode::SkipAll,
	false, 3, 250, 300, true, true, true, "", "", "", "", 
	ExtensionConfig::FilterMode::Disabled, L"", L"|", false
);


struct GptConfig {
	string url;
	string apiKey;
	int timeoutSecs;
	int numRetries;
	bool logRequest;
	vector<string> httpHeaders;

	GptConfig(string url_, string apiKey_, int timeoutSecs_, 
		int numRetries_, bool logRequest_, const vector<string>& httpHeaders_)
		: url(url_), apiKey(apiKey_), timeoutSecs(timeoutSecs_), 
			logRequest(logRequest_), numRetries(numRetries_), httpHeaders(httpHeaders_) { }
};


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
	string regexFormat(string pattern) const;
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
