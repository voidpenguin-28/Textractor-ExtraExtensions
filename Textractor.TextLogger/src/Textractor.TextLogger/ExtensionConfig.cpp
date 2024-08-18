
#include "ExtensionConfig.h"
#include "_Libraries/strhelper.h"
using FilterMode = ExtensionConfig::FilterMode;
using ConsoleClipboardMode = ExtensionConfig::ConsoleClipboardMode;

const wstring DISABLED_KEY = L"Disabled";
const wstring LOG_FILE_PATH_TEMPLATE_KEY = L"LogFilePathTemplate";
const wstring ACTIVE_THREAD_ONLY_KEY = L"ActiveThreadOnly";
const wstring SKIP_CONSOLE_AND_CLIPBOARD_KEY = L"SkipConsoleAndClipboard";
const wstring MSG_TEMPLATE = L"MsgTemplate";
const wstring ONLY_THREAD_NAME_KEY = L"OnlyThreadNameAsKey";
const wstring THREAD_KEY_FILTER_MODE_KEY = L"ThreadKeyFilterMode";
const wstring THREAD_KEY_FILTER_LIST_KEY = L"ThreadKeyFilterList";
const wstring THREAD_KEY_FILTER_LIST_DELIM_KEY = L"ThreadKeyFilterListDelim";


// *** PUBLIC

bool IniConfigRetriever::configSectionExists() const {
	auto ini = unique_ptr<IniContents>(_iniHandler.readIni());
	return ini->sectionExists(_iniSectionName);
}

void IniConfigRetriever::saveConfig(const ExtensionConfig& config, bool overrideIfExists) {
	auto ini = unique_ptr<IniContents>(_iniHandler.readIni());
	bool changed = false;

	changed |= setValue(*ini, THREAD_KEY_FILTER_LIST_DELIM_KEY, config.threadKeyFilterListDelim, overrideIfExists);
	changed |= setValue(*ini, THREAD_KEY_FILTER_LIST_KEY, config.threadKeyFilterList, overrideIfExists);
	changed |= setValue(*ini, THREAD_KEY_FILTER_MODE_KEY, config.threadKeyFilterMode, overrideIfExists);
	changed |= setValue(*ini, ONLY_THREAD_NAME_KEY, config.onlyThreadNameAsKey, overrideIfExists);
	changed |= setValue(*ini, MSG_TEMPLATE, config.msgTemplate, overrideIfExists);
	changed |= setValue(*ini, SKIP_CONSOLE_AND_CLIPBOARD_KEY, config.skipConsoleAndClipboard, overrideIfExists);
	changed |= setValue(*ini, ACTIVE_THREAD_ONLY_KEY, config.activeThreadOnly, overrideIfExists);
	changed |= setValue(*ini, LOG_FILE_PATH_TEMPLATE_KEY, config.logFilePathTemplate, overrideIfExists);
	changed |= setValue(*ini, DISABLED_KEY, config.disabled, overrideIfExists);

	if (changed) _iniHandler.saveIni(*ini);
}

ExtensionConfig IniConfigRetriever::getConfig(bool saveDefaultConfigIfNotExist) {
	if (saveDefaultConfigIfNotExist) setDefaultConfig(false);

	auto ini = unique_ptr<IniContents>(_iniHandler.readIni());
	ExtensionConfig defaultConfig = DefaultConfig;

	ExtensionConfig config = ExtensionConfig(
		getValOrDef(*ini, DISABLED_KEY, defaultConfig.disabled),
		getValOrDef(*ini, LOG_FILE_PATH_TEMPLATE_KEY, defaultConfig.logFilePathTemplate),
		getValOrDef(*ini, ACTIVE_THREAD_ONLY_KEY, defaultConfig.activeThreadOnly),
		getValOrDef<ConsoleClipboardMode>(*ini, 
			SKIP_CONSOLE_AND_CLIPBOARD_KEY, defaultConfig.skipConsoleAndClipboard),
		getValOrDef(*ini, MSG_TEMPLATE, defaultConfig.msgTemplate),
		getValOrDef(*ini, ONLY_THREAD_NAME_KEY, defaultConfig.onlyThreadNameAsKey),
		getValOrDef<FilterMode>(*ini, THREAD_KEY_FILTER_MODE_KEY, defaultConfig.threadKeyFilterMode),
		getValOrDef(*ini, THREAD_KEY_FILTER_LIST_KEY, defaultConfig.threadKeyFilterList),
		getValOrDef(*ini, THREAD_KEY_FILTER_LIST_DELIM_KEY, defaultConfig.threadKeyFilterListDelim)
	);

	return config;
}


// *** PRIVATE

bool IniConfigRetriever::configKeyExists(IniContents& ini, const wstring& key) const {
	return ini.keyExists(_iniSectionName, key);
}

bool IniConfigRetriever::setValue(IniContents& ini, const wstring& key,
	const wstring& value, bool overrideIfExists)
{
	return ini.setValue(_iniSectionName, key, value, overrideIfExists);
}

bool IniConfigRetriever::setValue(IniContents& ini, const wstring& key,
	const string& value, bool overrideIfExists)
{
	return setValue(ini, key, StrHelper::convertToW(value), overrideIfExists);
}

bool IniConfigRetriever::setValue(IniContents& ini, const wstring& key,
	int value, bool overrideIfExists)
{
	return setValue(ini, key, to_wstring(value), overrideIfExists);
}

void IniConfigRetriever::setDefaultConfig(bool overrideIfExists) {
	saveConfig(DefaultConfig, overrideIfExists);
}

wstring IniConfigRetriever::getValOrDef(IniContents& ini, const wstring& key, wstring defaultValue) const {
	return ini.getValue(_iniSectionName, key, defaultValue);
}

string IniConfigRetriever::getValOrDef(IniContents& ini, const wstring& key, string defaultValue) const {
	wstring value = getValOrDef(ini, key, StrHelper::convertToW(defaultValue));
	return StrHelper::convertFromW(value);
}

int IniConfigRetriever::getValOrDef(IniContents& ini, const wstring& key, int defaultValue) const {
	return ini.getValue(_iniSectionName, key, defaultValue);
}

template<typename T>
T IniConfigRetriever::getValOrDef(IniContents& ini, const wstring& key, int defaultValue) const {
	return static_cast<T>(getValOrDef(ini, key, defaultValue));
}
