
#include "ExtensionConfig.h"
#include "Libraries/strhelper.h"
using ConsoleClipboardMode = ExtensionConfig::ConsoleClipboardMode;

const wstring DISABLED_KEY = L"Disabled";
const wstring URL_TEMPLATE_KEY = L"UrlTemplate";
const wstring VN_IDS_KEY = L"VnIds";
const wstring VN_ID_DELIM_KEY = L"VnIdDelim";
const wstring MAPPING_MODE_KEY = L"MappingMode";
const wstring MIN_NAME_SIZE_KEY = L"MinNameCharSize";
const wstring ACTIVE_THREAD_ONLY_KEY = L"ActiveThreadOnly";
const wstring SKIP_CONSOLE_AND_CLIPBOARD_KEY = L"SkipConsoleAndClipboard";
const wstring RELOAD_CACHE_ON_LAUNCH_KEY = L"ReloadCacheOnLaunch";
const wstring CUSTOM_CURL_PATH_KEY = L"CustomCurlPath";


// *** PUBLIC

bool IniConfigRetriever::configSectionExists() const {
	auto ini = unique_ptr<IniContents>(_iniHandler.readIni());
	return ini->sectionExists(_iniSectionName);
}

void IniConfigRetriever::saveConfig(const ExtensionConfig& config, bool overrideIfExists) {
	auto ini = unique_ptr<IniContents>(_iniHandler.readIni());
	bool changed = false;

	changed |= setValue(*ini, CUSTOM_CURL_PATH_KEY, config.customCurlPath, overrideIfExists);
	changed |= setValue(*ini, RELOAD_CACHE_ON_LAUNCH_KEY, config.reloadCacheOnLaunch, overrideIfExists);
	changed |= setValue(*ini, SKIP_CONSOLE_AND_CLIPBOARD_KEY, config.skipConsoleAndClipboard, overrideIfExists);
	changed |= setValue(*ini, ACTIVE_THREAD_ONLY_KEY, config.activeThreadOnly, overrideIfExists);
	changed |= setValue(*ini, MIN_NAME_SIZE_KEY, config.minNameCharSize, overrideIfExists);
	changed |= setValue(*ini, MAPPING_MODE_KEY, config.mappingMode, overrideIfExists);
	changed |= setValue(*ini, VN_ID_DELIM_KEY, config.vnIdDelim, overrideIfExists);
	changed |= setValue(*ini, VN_IDS_KEY, config.vnIds, overrideIfExists);
	changed |= setValue(*ini, URL_TEMPLATE_KEY, config.urlTemplate, overrideIfExists);
	changed |= setValue(*ini, DISABLED_KEY, config.disabled, overrideIfExists);

	if (changed) _iniHandler.saveIni(*ini);
}

ExtensionConfig IniConfigRetriever::getConfig(bool saveDefaultConfigIfNotExist) {
	if (saveDefaultConfigIfNotExist) setDefaultConfig(false);

	auto ini = unique_ptr<IniContents>(_iniHandler.readIni());
	ExtensionConfig defaultConfig = DefaultConfig;

	ExtensionConfig config = ExtensionConfig(
		getValOrDef(*ini, DISABLED_KEY, DefaultConfig.disabled),
		getValOrDef(*ini, URL_TEMPLATE_KEY, DefaultConfig.urlTemplate),
		StrHelper::trim<wchar_t>(getValOrDef(*ini, VN_IDS_KEY, DefaultConfig.vnIds), L"\""),
		getValOrDef(*ini, VN_ID_DELIM_KEY, DefaultConfig.vnIdDelim),
		getValOrDef<MappingMode>(*ini, MAPPING_MODE_KEY, DefaultConfig.mappingMode),
		getValOrDef(*ini, MIN_NAME_SIZE_KEY, DefaultConfig.minNameCharSize),
		getValOrDef(*ini, ACTIVE_THREAD_ONLY_KEY, DefaultConfig.activeThreadOnly),
		getValOrDef<ConsoleClipboardMode>(*ini, 
			SKIP_CONSOLE_AND_CLIPBOARD_KEY, DefaultConfig.skipConsoleAndClipboard),
		getValOrDef(*ini, RELOAD_CACHE_ON_LAUNCH_KEY, DefaultConfig.reloadCacheOnLaunch),
		getValOrDef(*ini, CUSTOM_CURL_PATH_KEY, DefaultConfig.customCurlPath)
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
