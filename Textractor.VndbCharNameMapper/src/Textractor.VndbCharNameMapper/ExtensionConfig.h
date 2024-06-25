#pragma once

#include "Libraries/inihandler.h"
#include <string>
using namespace std;

struct ExtensionConfig {
	enum MappingMode { None, Name, NameAndGender };

	const bool disabled;
	const string urlTemplate;
	const wstring vnIds;
	const wstring vnIdDelim;
	const MappingMode mappingMode;
	const int minNameCharSize;
	const bool activeThreadOnly;
	const int skipConsoleAndClipboard;
	const bool reloadCacheOnLaunch;
	const string customCurlPath;

	ExtensionConfig(bool disabled_, string urlTemplate_, wstring vnIds_, wstring vnIdDelim_, 
		MappingMode mappingMode_, int minNameCharSize_, bool activeThreadOnly_,
		int skipConsoleAndClipboard_, bool reloadCacheOnLaunch_, string customCurlPath_)
		: disabled(disabled_), urlTemplate(urlTemplate_), vnIds(vnIds_), vnIdDelim(vnIdDelim_),
			mappingMode(mappingMode_), minNameCharSize(minNameCharSize_), 
			activeThreadOnly(activeThreadOnly_),skipConsoleAndClipboard(skipConsoleAndClipboard_), 
			reloadCacheOnLaunch(reloadCacheOnLaunch_), customCurlPath(customCurlPath_) { }
};

using MappingMode = ExtensionConfig::MappingMode;

static const ExtensionConfig DefaultConfig = ExtensionConfig(
	false, "https://vndb.org/{0}/chars", L"", L"|", ExtensionConfig::MappingMode::Name, 2, true, 1, false, ""
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

