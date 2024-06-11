
#include "ExtensionConfig.h"
#include "../_Libraries/strhelper.h"
using FilterMode = ExtensionConfig::FilterMode;


const wstring DISABLED_KEY = L"Disabled";
const wstring URL_KEY = L"Url";
const wstring APIKEY_KEY = L"ApiKey";
const wstring MODEL_KEY = L"Model";
const wstring TIMEOUT_SECS_KEY = L"TimeoutSecs";
const wstring NUM_RETRIES_KEY = L"NumRetries";
const wstring SYS_MSG_PREFIX_KEY = L"SysMsgPrefix";
const wstring USER_MSG_PREFIX_KEY = L"UserMsgPrefix";
const wstring ACTIVE_THREAD_ONLY_KEY = L"ActiveThreadOnly";
const wstring SKIP_CONSOLE_AND_CLIPBOARD_KEY = L"SkipConsoleAndClipboard";
const wstring USE_HIST_FOR_NONACTIVE_THREADS_KEY = L"UseHistoryForNonActiveThreads";
const wstring MSG_HISTORY_COUNT_KEY = L"MsgHistoryCount";
const wstring HIST_SOFT_CHAR_LIMIT_KEY = L"HistorySoftCharLimit";
const wstring MSG_CHAR_LIMIT_KEY = L"MsgCharLimit";
const wstring SKIP_ASCII_TEXT_KEY = L"SkipAsciiText";
const wstring SKIP_IF_ZERO_WD_SPACE_KEY = L"SkipIfZeroWidthSpace";
const wstring SHOW_ERR_MSG_KEY = L"ShowErrMsg";
const wstring CUSTOM_REQUEST_TEMPLATE_KEY = L"CustomRequestTemplate";
const wstring CUSTOM_RESPONSE_MSG_REGEX_KEY = L"CustomResponseMsgRegex";
const wstring CUSTOM_ERROR_MSG_REGEX_KEY = L"CustomErrorMsgRegex";
const wstring CUSTOM_HTTP_HEADERS_KEY = L"CustomHttpHeaders";
const wstring THREAD_KEY_FILTER_MODE_KEY = L"ThreadKeyFilterMode";
const wstring THREAD_KEY_FILTER_LIST_KEY = L"ThreadKeyFilterList";
const wstring THREAD_KEY_FILTER_LIST_DELIM_KEY = L"ThreadKeyFilterListDelim";
const wstring CUSTOM_CURL_PATH_KEY = L"CustomCurlPath";
const wstring DEBUG_MODE_KEY = L"DebugMode";


// *** PUBLIC

bool IniConfigRetriever::configSectionExists() const {
	auto ini = unique_ptr<IniContents>(_iniHandler.readIni());
	return ini->sectionExists(_iniSectionName);
}

bool IniConfigRetriever::configKeyExists(IniContents& ini, const wstring& key) const {
	return ini.keyExists(_iniSectionName, key);
}

void IniConfigRetriever::saveConfig(const ExtensionConfig& config, bool overrideIfExists) const {
	auto ini = unique_ptr<IniContents>(_iniHandler.readIni());
	bool changed = false;

	changed |= setValue(*ini, DEBUG_MODE_KEY, config.debugMode, overrideIfExists);
	changed |= setValue(*ini, CUSTOM_CURL_PATH_KEY, config.customCurlPath, overrideIfExists);
	changed |= setValue(*ini, THREAD_KEY_FILTER_LIST_DELIM_KEY, config.threadKeyFilterListDelim, overrideIfExists);
	changed |= setValue(*ini, THREAD_KEY_FILTER_LIST_KEY, config.threadKeyFilterList, overrideIfExists);
	changed |= setValue(*ini, THREAD_KEY_FILTER_MODE_KEY, config.threadKeyFilterMode, overrideIfExists);
	changed |= setValue(*ini, CUSTOM_HTTP_HEADERS_KEY, config.customHttpHeaders, overrideIfExists);
	changed |= setValue(*ini, CUSTOM_ERROR_MSG_REGEX_KEY, config.customErrorMsgRegex, overrideIfExists);
	changed |= setValue(*ini, CUSTOM_RESPONSE_MSG_REGEX_KEY, config.customResponseMsgRegex, overrideIfExists);
	changed |= setValue(*ini, CUSTOM_REQUEST_TEMPLATE_KEY, config.customRequestTemplate, overrideIfExists);
	changed |= setValue(*ini, SHOW_ERR_MSG_KEY, config.showErrMsg, overrideIfExists);
	changed |= setValue(*ini, SKIP_IF_ZERO_WD_SPACE_KEY, config.skipIfZeroWidthSpace, overrideIfExists);
	changed |= setValue(*ini, SKIP_ASCII_TEXT_KEY, config.skipAsciiText, overrideIfExists);
	changed |= setValue(*ini, MSG_CHAR_LIMIT_KEY, config.msgCharLimit, overrideIfExists);
	changed |= setValue(*ini, HIST_SOFT_CHAR_LIMIT_KEY, config.historySoftCharLimit, overrideIfExists);
	changed |= setValue(*ini, MSG_HISTORY_COUNT_KEY, config.msgHistoryCount, overrideIfExists);
	changed |= setValue(*ini, USE_HIST_FOR_NONACTIVE_THREADS_KEY, config.useHistoryForNonActiveThreads, overrideIfExists);
	changed |= setValue(*ini, SKIP_CONSOLE_AND_CLIPBOARD_KEY, config.skipConsoleAndClipboard, overrideIfExists);
	changed |= setValue(*ini, ACTIVE_THREAD_ONLY_KEY, config.activeThreadOnly, overrideIfExists);
	changed |= setValue(*ini, USER_MSG_PREFIX_KEY, config.userMsgPrefix, overrideIfExists);
	changed |= setValue(*ini, SYS_MSG_PREFIX_KEY, config.sysMsgPrefix, overrideIfExists);
	changed |= setValue(*ini, NUM_RETRIES_KEY, config.numRetries, overrideIfExists);
	changed |= setValue(*ini, TIMEOUT_SECS_KEY, config.timeoutSecs, overrideIfExists);
	changed |= setValue(*ini, MODEL_KEY, config.model, overrideIfExists);
	changed |= setValue(*ini, APIKEY_KEY, config.apiKey, overrideIfExists);
	changed |= setValue(*ini, URL_KEY, config.url, overrideIfExists);
	changed |= setValue(*ini, DISABLED_KEY, config.disabled, overrideIfExists);

	if(changed) _iniHandler.saveIni(*ini);
}

ExtensionConfig IniConfigRetriever::getConfig(bool saveDefaultConfigIfNotExist) const {
	if (saveDefaultConfigIfNotExist) setDefaultConfig(false);

	auto ini = unique_ptr<IniContents>(_iniHandler.readIni());
	ExtensionConfig defaultConfig = DefaultConfig;

	ExtensionConfig config = ExtensionConfig(
		getValOrDef(*ini, DISABLED_KEY, defaultConfig.disabled),
		getValOrDef(*ini, URL_KEY, defaultConfig.url),
		getValOrDef(*ini, APIKEY_KEY, defaultConfig.apiKey),
		getValOrDef(*ini, MODEL_KEY, defaultConfig.model),
		getValOrDef(*ini, TIMEOUT_SECS_KEY, defaultConfig.timeoutSecs),
		getValOrDef(*ini, NUM_RETRIES_KEY, defaultConfig.numRetries),
		getValOrDef(*ini, SYS_MSG_PREFIX_KEY, defaultConfig.sysMsgPrefix),
		getValOrDef(*ini, USER_MSG_PREFIX_KEY, defaultConfig.userMsgPrefix),
		getValOrDef(*ini, ACTIVE_THREAD_ONLY_KEY, defaultConfig.activeThreadOnly),
		getValOrDef(*ini, SKIP_CONSOLE_AND_CLIPBOARD_KEY, defaultConfig.skipConsoleAndClipboard),
		getValOrDef(*ini, USE_HIST_FOR_NONACTIVE_THREADS_KEY, defaultConfig.useHistoryForNonActiveThreads),
		getValOrDef(*ini, MSG_HISTORY_COUNT_KEY, defaultConfig.msgHistoryCount),
		getValOrDef(*ini, HIST_SOFT_CHAR_LIMIT_KEY, defaultConfig.historySoftCharLimit),
		getValOrDef(*ini, MSG_CHAR_LIMIT_KEY, defaultConfig.msgCharLimit),
		getValOrDef(*ini, SKIP_ASCII_TEXT_KEY, defaultConfig.skipAsciiText),
		getValOrDef(*ini, SKIP_IF_ZERO_WD_SPACE_KEY, defaultConfig.skipIfZeroWidthSpace),
		getValOrDef(*ini, SHOW_ERR_MSG_KEY, defaultConfig.showErrMsg),
		getValOrDef(*ini, CUSTOM_REQUEST_TEMPLATE_KEY, defaultConfig.customRequestTemplate),
		getValOrDef(*ini, CUSTOM_RESPONSE_MSG_REGEX_KEY, defaultConfig.customResponseMsgRegex),
		getValOrDef(*ini, CUSTOM_ERROR_MSG_REGEX_KEY, defaultConfig.customErrorMsgRegex),
		getValOrDef(*ini, CUSTOM_HTTP_HEADERS_KEY, defaultConfig.customHttpHeaders),
		static_cast<FilterMode>(getValOrDef(*ini, THREAD_KEY_FILTER_MODE_KEY, defaultConfig.threadKeyFilterMode)),
		getValOrDef(*ini, THREAD_KEY_FILTER_LIST_KEY, defaultConfig.threadKeyFilterList),
		getValOrDef(*ini, THREAD_KEY_FILTER_LIST_DELIM_KEY, defaultConfig.threadKeyFilterListDelim),
		getValOrDef(*ini, CUSTOM_CURL_PATH_KEY, defaultConfig.customCurlPath),
		getValOrDef(*ini, DEBUG_MODE_KEY, defaultConfig.debugMode)
	);

	return config;
}


// *** PRIVATE

bool IniConfigRetriever::setValue(IniContents& ini, const wstring& key, 
	const wstring& value, bool overrideIfExists) const
{
	return ini.setValue(_iniSectionName, key, value, overrideIfExists);
}

bool IniConfigRetriever::setValue(IniContents& ini, const wstring& key,
	const string& value, bool overrideIfExists) const
{
	return setValue(ini, key, StrHelper::convertToW(value), overrideIfExists);
}

bool IniConfigRetriever::setValue(IniContents& ini, const wstring& key,
	int value, bool overrideIfExists) const
{
	return setValue(ini, key, to_wstring(value), overrideIfExists);
}

void IniConfigRetriever::setDefaultConfig(bool overrideIfExists) const {
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
