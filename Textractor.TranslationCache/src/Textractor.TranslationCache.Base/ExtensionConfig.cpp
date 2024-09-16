
#include "ExtensionConfig.h"
#include "_Libraries/strhelper.h"
using DisabledMode = ExtensionConfig::DisabledMode;
using SkippingStrategy = ExtensionConfig::SkippingStrategy;
using ConsoleClipboardMode = ExtensionConfig::ConsoleClipboardMode;
using FilterMode = ExtensionConfig::FilterMode;

const wstring DISABLED_MODE_KEY = L"Disabled";
const wstring CACHE_FILE_PATH_KEY = L"CacheFilePath";
const wstring SKIPPING_STRATEGY_KEY = L"SkippingStrategy";
const wstring ACTIVE_THREAD_ONLY_KEY = L"ActiveThreadOnly";
const wstring SKIP_CONSOLE_AND_CLIPBOARD_KEY = L"SkipConsoleAndClipboard";
const wstring CACHE_FILE_LIMIT_MB_KEY = L"CacheFileLimitMb";
const wstring CACHE_LINE_LENGTH_LIMIT_KEY = L"CacheLineLengthLimit";
const wstring CLEAR_CACHE_ON_UNLOAD_KEY = L"ClearCacheOnUnload";
const wstring THREAD_KEY_FILTER_MODE_KEY = L"ThreadKeyFilterMode";
const wstring THREAD_KEY_FILTER_LIST_KEY = L"ThreadKeyFilterList";
const wstring THREAD_KEY_FILTER_LIST_DELIM_KEY = L"ThreadKeyFilterListDelim";
const wstring DEBUG_MODE_KEY = L"DebugMode";


// *** PUBLIC

bool IniConfigRetriever::configSectionExists() const {
	auto ini = unique_ptr<IniContents>(_iniHandler.readIni());
	return ini->sectionExists(_iniSectionName);
}

void IniConfigRetriever::saveConfig(const ExtensionConfig& config, bool overrideIfExists) {
	auto ini = unique_ptr<IniContents>(_iniHandler.readIni());
	bool changed = false;

	changed |= setValue(*ini, DEBUG_MODE_KEY, config.debugMode, overrideIfExists);
	changed |= setValue(*ini, THREAD_KEY_FILTER_LIST_DELIM_KEY, config.threadKeyFilterListDelim, overrideIfExists);
	changed |= setValue(*ini, THREAD_KEY_FILTER_LIST_KEY, config.threadKeyFilterList, overrideIfExists);
	changed |= setValue(*ini, THREAD_KEY_FILTER_MODE_KEY, config.threadKeyFilterMode, overrideIfExists);
	changed |= setValue(*ini, CLEAR_CACHE_ON_UNLOAD_KEY, config.clearCacheOnUnload, overrideIfExists);
	changed |= setValue(*ini, CACHE_LINE_LENGTH_LIMIT_KEY, config.cacheLineLengthLimit, overrideIfExists);
	changed |= setValue(*ini, CACHE_FILE_LIMIT_MB_KEY, config.cacheFileLimitMb, overrideIfExists);
	changed |= setValue(*ini, SKIP_CONSOLE_AND_CLIPBOARD_KEY, config.skipConsoleAndClipboard, overrideIfExists);
	changed |= setValue(*ini, ACTIVE_THREAD_ONLY_KEY, config.activeThreadOnly, overrideIfExists);
	changed |= setValue(*ini, SKIPPING_STRATEGY_KEY, config.skippingStrategy, overrideIfExists);
	changed |= setValue(*ini, CACHE_FILE_PATH_KEY, config.cacheFilePath, overrideIfExists);
	changed |= setValue(*ini, DISABLED_MODE_KEY, config.disabledMode, overrideIfExists);
	
	if (changed) _iniHandler.saveIni(*ini);
}

ExtensionConfig IniConfigRetriever::getConfig(bool saveDefaultConfigIfNotExist) {
	if (saveDefaultConfigIfNotExist) setDefaultConfig(false);

	auto ini = unique_ptr<IniContents>(_iniHandler.readIni());
	ExtensionConfig defaultConfig = DefaultConfig;

	ExtensionConfig config = ExtensionConfig(
		getValOrDef<DisabledMode>(*ini, DISABLED_MODE_KEY, defaultConfig.disabledMode),
		getValOrDef(*ini, CACHE_FILE_PATH_KEY, defaultConfig.cacheFilePath),
		getValOrDef<SkippingStrategy>(*ini, SKIPPING_STRATEGY_KEY, defaultConfig.skippingStrategy),
		getValOrDef(*ini, ACTIVE_THREAD_ONLY_KEY, defaultConfig.activeThreadOnly),
		getValOrDef<ConsoleClipboardMode>(*ini, 
			SKIP_CONSOLE_AND_CLIPBOARD_KEY, defaultConfig.skipConsoleAndClipboard),
		getValOrDef(*ini, CACHE_FILE_LIMIT_MB_KEY, defaultConfig.cacheFileLimitMb),
		getValOrDef(*ini, CACHE_LINE_LENGTH_LIMIT_KEY, defaultConfig.cacheLineLengthLimit),
		getValOrDef(*ini, CLEAR_CACHE_ON_UNLOAD_KEY, defaultConfig.clearCacheOnUnload),
		getValOrDef<FilterMode>(*ini, THREAD_KEY_FILTER_MODE_KEY, defaultConfig.threadKeyFilterMode),
		getValOrDef(*ini, THREAD_KEY_FILTER_LIST_KEY, defaultConfig.threadKeyFilterList),
		getValOrDef(*ini, THREAD_KEY_FILTER_LIST_DELIM_KEY, defaultConfig.threadKeyFilterListDelim),
		getValOrDef(*ini, DEBUG_MODE_KEY, defaultConfig.debugMode)
	);

	return config;
}


// *** PRIVATE

bool IniConfigRetriever::configKeyExists(IniContents& ini, const wstring& key) const {
	return ini.keyExists(_iniSectionName, key);
}

bool IniConfigRetriever::setValue(IniContents& ini, 
	const wstring& key, const wstring& value, bool overrideIfExists)
{
	return ini.setValue(_iniSectionName, key, value, overrideIfExists);
}

bool IniConfigRetriever::setValue(IniContents& ini, 
	const wstring& key, const string& value, bool overrideIfExists)
{
	return setValue(ini, key, StrHelper::convertToW(value), overrideIfExists);
}

bool IniConfigRetriever::setValue(IniContents& ini, 
	const wstring& key, int value, bool overrideIfExists)
{
	return setValue(ini, key, to_wstring(value), overrideIfExists);
}

bool IniConfigRetriever::setValue(IniContents& ini, 
	const wstring& key, double value, bool overrideIfExists)
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

double IniConfigRetriever::getValOrDef(IniContents& ini, const wstring& key, double defaultValue) const {
	return ini.getValue(_iniSectionName, key, defaultValue);
}

template<typename T>
T IniConfigRetriever::getValOrDef(IniContents& ini, const wstring& key, int defaultValue) const {
	return static_cast<T>(getValOrDef(ini, key, defaultValue));
}
