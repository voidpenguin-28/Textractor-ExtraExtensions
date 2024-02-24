#pragma once

#include "Libraries/inihandler.h"
#include <string>
#include <unordered_set>
using namespace std;

extern const char* INI_FILE_NAME_DEFAULT;
extern const char* INI_SECTION_NAME_DEFAULT;

struct ExtensionConfig {
	enum FilterMode { Disabled = 0, Blacklist, Whitelist };

	const bool disabled;
	const wstring logFilePathTemplate; // {0}: process name; {1}: thread key; {2}: thread name
	const bool activeThreadOnly;
	const int skipConsoleAndClipboard;
	const wstring msgTemplate; // {0}: sentence; {1}: process name; {2}: thread key; {3}: thread name; {4}: current date time
	const bool onlyThreadNameAsKey;
	const FilterMode threadKeyFilterMode;
	const wstring threadKeyFilterList;
	const wstring threadKeyFilterListDelim;

	ExtensionConfig(const bool disabled_, const wstring& logFilePathTemplate_, const bool activeThreadOnly_, 
		const int skipConsoleAndClipboard_, const wstring& msgTemplate_, const bool onlyThreadNameAsKey_,
		const FilterMode threadKeyFilterMode_, const wstring& threadKeyFilterList_, 
		const wstring& threadKeyFilterListDelim_)
		: disabled(disabled_), logFilePathTemplate(logFilePathTemplate_), activeThreadOnly(activeThreadOnly_), 
			skipConsoleAndClipboard(skipConsoleAndClipboard_), msgTemplate(msgTemplate_),
			onlyThreadNameAsKey(onlyThreadNameAsKey_), threadKeyFilterMode(threadKeyFilterMode_), 
			threadKeyFilterList(threadKeyFilterList_), threadKeyFilterListDelim(threadKeyFilterListDelim_) { }
};

const ExtensionConfig DefaultConfig = ExtensionConfig(
	false, L"logs\\{0}\\{1}-log.txt", true, true, L"[{4}][{1}][{2}] {0}", 
	false, ExtensionConfig::FilterMode::Disabled, L"", L"|"
);


class ConfigRetriever {
public:
	virtual ~ConfigRetriever() { }
	virtual bool configSectionExists() const = 0;
	virtual ExtensionConfig getConfig(bool saveDefaultConfigIfNotExist = true) const = 0;
	virtual void saveConfig(const ExtensionConfig& config, bool overrideIfExists) const = 0;
};


class IniConfigRetriever : public ConfigRetriever {
public:
	IniConfigRetriever(const string& iniFileName, const wstring& iniSectionName)
		: _iniFileName(iniFileName), _iniSectionName(iniSectionName), _iniHandler(iniFileName) { }

	bool configSectionExists() const override;
	ExtensionConfig getConfig(bool saveDefaultConfigIfNotExist = true) const override;
	void saveConfig(const ExtensionConfig& config, bool overrideIfExists) const override;
private:
	const IniFileHandler _iniHandler;
	const string _iniFileName;
	const wstring _iniSectionName;

	bool configKeyExists(IniContents& ini, const wstring& key) const;
	bool setValue(IniContents& ini, const wstring& key, const wstring& value, bool overrideIfExists) const;
	bool setValue(IniContents& ini, const wstring& key, const string& value, bool overrideIfExists) const;
	bool setValue(IniContents& ini, const wstring& key, int value, bool overrideIfExists) const;
	void setDefaultConfig(bool overrideIfExists) const;
	wstring getValOrDef(IniContents& ini, const wstring& key, wstring defaultValue) const;
	string getValOrDef(IniContents& ini, const wstring& key, string defaultValue) const;
	int getValOrDef(IniContents& ini, const wstring& key, int defaultValue) const;
};
