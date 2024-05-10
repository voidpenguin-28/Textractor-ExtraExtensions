
#include "ExtensionConfig.h"
#include "Libraries/stringconvert.h"

const wstring DISABLED_KEY = L"Disabled";
const wstring SCRIPT_PATH_KEY = L"ScriptPath";
const wstring LOG_DIR_PATH_KEY = L"LogDirPath";
const wstring LOG_LEVEL_KEY = L"LogLevel";
const wstring APPEND_ERR_MSG_KEY = L"AppendErrMsg";
const wstring ACTIVE_THREAD_ONLY_KEY = L"ActiveThreadOnly";
const wstring SKIP_CONSOLE_AND_CLIPBOARD_KEY = L"SkipConsoleAndClipboard";
const wstring RELOAD_ON_SCRIPT_MOD_KEY = L"ReloadOnScriptModified";
const wstring PIP_PACKAGE_INSTALL_MODE_KEY = L"PipPackageInstallMode";
const wstring PIP_REQUIREMENTS_TXT_PATH_KEY = L"PipRequirementsTxtPath";
const wstring SHOW_LOG_CONSOLE_KEY = L"ShowLogConsole";
const wstring SCRIPT_CUSTOM_VARS_KEY = L"ScriptCustomVars";
const wstring SCRIPT_CUSTOM_VARS_DELIM_KEY = L"ScriptCustomVarsDelim";
const wstring CUSTOM_PYTHON_PATH_KEY = L"CustomPythonPath";


// *** PUBLIC

bool IniConfigRetriever::configSectionExists() {
	auto ini = unique_ptr<IniContents>(_iniHandler.readIni());
	return ini->sectionExists(_iniSectionName);
}

bool IniConfigRetriever::configKeyExists(IniContents& ini, const wstring& key) {
	return ini.keyExists(_iniSectionName, key);
}

void IniConfigRetriever::saveConfig(const ExtensionConfig& config, bool overrideIfExists) {
	auto ini = unique_ptr<IniContents>(_iniHandler.readIni());
	bool changed = false;

	changed |= setValue(*ini, CUSTOM_PYTHON_PATH_KEY, config.customPythonPath, overrideIfExists);
	changed |= setValue(*ini, SCRIPT_CUSTOM_VARS_DELIM_KEY, config.scriptCustomVarsDelim, overrideIfExists);
	changed |= setValue(*ini, SCRIPT_CUSTOM_VARS_KEY, config.scriptCustomVars, overrideIfExists);
	changed |= setValue(*ini, SHOW_LOG_CONSOLE_KEY, config.showLogConsole, overrideIfExists);
	changed |= setValue(*ini, PIP_REQUIREMENTS_TXT_PATH_KEY, config.pipRequirementsTxtPath, overrideIfExists);
	changed |= setValue(*ini, PIP_PACKAGE_INSTALL_MODE_KEY, config.pipPackageInstallMode, overrideIfExists);
	changed |= setValue(*ini, RELOAD_ON_SCRIPT_MOD_KEY, config.reloadOnScriptModified, overrideIfExists);
	changed |= setValue(*ini, SKIP_CONSOLE_AND_CLIPBOARD_KEY, config.skipConsoleAndClipboard, overrideIfExists);
	changed |= setValue(*ini, ACTIVE_THREAD_ONLY_KEY, config.activeThreadOnly, overrideIfExists);
	changed |= setValue(*ini, APPEND_ERR_MSG_KEY, config.appendErrMsg, overrideIfExists);
	changed |= setValue(*ini, LOG_LEVEL_KEY, config.logLevel, overrideIfExists);
	changed |= setValue(*ini, LOG_DIR_PATH_KEY, config.logDirPath, overrideIfExists);
	changed |= setValue(*ini, SCRIPT_PATH_KEY, config.scriptPath, overrideIfExists);
	changed |= setValue(*ini, DISABLED_KEY, config.disabled, overrideIfExists);

	if (changed) _iniHandler.saveIni(*ini);
}

ExtensionConfig IniConfigRetriever::getConfig(bool saveDefaultConfigIfNotExist) {
	if (saveDefaultConfigIfNotExist) setDefaultConfig(false);

	auto ini = unique_ptr<IniContents>(_iniHandler.readIni());
	ExtensionConfig defaultConfig = DefaultConfig;

	ExtensionConfig config = ExtensionConfig(
		getValOrDef(*ini, DISABLED_KEY, defaultConfig.disabled),
		getValOrDef(*ini, SCRIPT_PATH_KEY, defaultConfig.scriptPath),
		getValOrDef(*ini, LOG_DIR_PATH_KEY, defaultConfig.logDirPath),
		getValOrDef(*ini, LOG_LEVEL_KEY, defaultConfig.logLevel),
		getValOrDef(*ini, APPEND_ERR_MSG_KEY, defaultConfig.appendErrMsg),
		getValOrDef(*ini, ACTIVE_THREAD_ONLY_KEY, defaultConfig.activeThreadOnly),
		getValOrDef(*ini, SKIP_CONSOLE_AND_CLIPBOARD_KEY, defaultConfig.skipConsoleAndClipboard),
		getValOrDef(*ini, RELOAD_ON_SCRIPT_MOD_KEY, defaultConfig.reloadOnScriptModified),
		getValOrDef(*ini, PIP_PACKAGE_INSTALL_MODE_KEY, defaultConfig.pipPackageInstallMode),
		getValOrDef(*ini, PIP_REQUIREMENTS_TXT_PATH_KEY, defaultConfig.pipRequirementsTxtPath),
		getValOrDef(*ini, SHOW_LOG_CONSOLE_KEY, defaultConfig.showLogConsole),
		unenclose(getValOrDef(*ini, SCRIPT_CUSTOM_VARS_KEY, defaultConfig.scriptCustomVars), '"'),
		getValOrDef(*ini, SCRIPT_CUSTOM_VARS_DELIM_KEY, defaultConfig.scriptCustomVarsDelim),
		getValOrDef(*ini, CUSTOM_PYTHON_PATH_KEY, defaultConfig.customPythonPath)
	);

	return config;
}


// *** PRIVATE

bool IniConfigRetriever::setValue(IniContents& ini, 
	const wstring& key, const wstring& value, bool overrideIfExists)
{
	return ini.setValue(_iniSectionName, key, value, overrideIfExists);
}

bool IniConfigRetriever::setValue(IniContents& ini, 
	const wstring& key, const string& value, bool overrideIfExists)
{
	return setValue(ini, key, convertToW(value), overrideIfExists);
}

bool IniConfigRetriever::setValue(IniContents& ini, 
	const wstring& key, int value, bool overrideIfExists)
{
	return setValue(ini, key, to_wstring(value), overrideIfExists);
}

bool IniConfigRetriever::setValue(IniContents& ini,
	const wstring& key, Logger::Level value, bool overrideIfExists)
{
	string logLevelStr = logLevelToStr(value);
	return setValue(ini, key, logLevelStr, overrideIfExists);
}

string IniConfigRetriever::logLevelToStr(Logger::Level logLevel) {
	switch (logLevel) {
	case Logger::Info: return "Info";
	case Logger::Warning: return "Warning";
	case Logger::Error: return "Error";
	case Logger::Fatal: return "Fatal";
	default: return "Debug";
	}
}

void IniConfigRetriever::setDefaultConfig(bool overrideIfExists) {
	saveConfig(DefaultConfig, overrideIfExists);
}

wstring IniConfigRetriever::getValOrDef(IniContents& ini, const wstring& key, wstring defaultValue) {
	return ini.getValue(_iniSectionName, key, defaultValue);
}

string IniConfigRetriever::getValOrDef(IniContents& ini, const wstring& key, string defaultValue) {
	wstring value = getValOrDef(ini, key, convertToW(defaultValue));
	return convertFromW(value);
}

int IniConfigRetriever::getValOrDef(IniContents& ini, const wstring& key, int defaultValue) {
	return ini.getValue(_iniSectionName, key, defaultValue);
}

Logger::Level IniConfigRetriever::getValOrDef(IniContents& ini, const wstring& key, Logger::Level defaultValue) {
	string logLevelStr = getValOrDef(ini, key, "");
	return strToLogLevel(logLevelStr, defaultValue);
}

Logger::Level IniConfigRetriever::strToLogLevel(const string& logLevelStr, Logger::Level defaultValue) {
	if (logLevelStr == "Debug") return Logger::Debug;
	else if (logLevelStr == "Info") return Logger::Info;
	else if (logLevelStr == "Warning") return Logger::Warning;
	else if (logLevelStr == "Error") return Logger::Error;
	else if (logLevelStr == "Fatal") return Logger::Fatal;
	else return defaultValue;
}

string IniConfigRetriever::unenclose(string str, const char encloseCh) {
	if (str.length() < 2) return str;

	return (str[0] == encloseCh && str[str.length() - 1] == encloseCh) ?
		str.substr(1, str.length() - 2) : str;
}
