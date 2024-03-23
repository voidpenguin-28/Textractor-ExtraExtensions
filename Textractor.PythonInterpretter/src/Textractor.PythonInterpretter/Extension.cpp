
#include "logging/Loggers.h"
#include "Libraries/winmsg.h"
#include "Libraries/stringconvert.h"
#include "containers/ExtensionDepsContainer.h"
#include "environment/DirectoryCreator.h"
#include "Extension.h"
#include "ExtensionConfig.h"
#include <functional>
#include <memory>
#include <mutex>
#include <string> 
#include <windows.h>


string _moduleName;
ExtensionDepsContainer* _deps = nullptr;
atomic<bool> _disabledByErr = false;
bool ProcessSentenceBase(ScriptManager& scriptManager, wstring& sentence, SentenceInfoWrapper& sentInfoWrapper);
bool meetsExecutionRequirements(SentenceInfoWrapper& sentInfoWrapper, const ExtensionConfig& config);
bool meetsConsoleAndClipboardRequirements(SentenceInfoWrapper& sentInfoWrapper, const ExtensionConfig& config);
bool validScriptState(const ScriptManager& scriptManager);


inline string getModuleName(const HMODULE& handle) {
	try {
		wchar_t buffer[1024];
		GetModuleFileName(handle, buffer, sizeof(buffer) / sizeof(wchar_t));

		string module = convertFromW(buffer);
		size_t pathDelimIndex = module.rfind('\\');
		if (pathDelimIndex != string::npos) module = module.substr(pathDelimIndex + 1);

		size_t extIndex = module.rfind('.');
		if (extIndex != string::npos) module = module.erase(extIndex);

		return module;
	}
	catch (exception& ex) {
		string msg = "Failed to retrieve extension name. Exception: " + string(ex.what());
		showErrorMessage(msg.c_str(), "PythonInterpretter");
		throw;
	}
}

inline ExtensionConfig getConfig(bool saveDefaultConfigIfNotExist) {
	return _deps->getConfigRetriever().getConfig(saveDefaultConfigIfNotExist);
}

inline ScriptManager& getScriptManager() {
	return _deps->getScriptManager();
}

inline void loadScript(ScriptManager& scriptManager, const ExtensionConfig& config, 
	bool resetScript, string beforeLoadMsg, string afterLoadMsg)
{
	Locker& loadLocker = _deps->getLoadLocker();

	bool tookLock = loadLocker.tryLock([&scriptManager, &config, resetScript, &beforeLoadMsg, &afterLoadMsg]() {
		Logger& msgBoxLogger = _deps->getMsgBoxLogger();
		Logger::Level logLevel = resetScript ? Logger::Info : Logger::Debug;
		msgBoxLogger.log(logLevel, beforeLoadMsg);

		scriptManager.loadPythonScript(config.scriptPath, resetScript);
		if (scriptManager.isScriptLoaded()) msgBoxLogger.log(logLevel, afterLoadMsg);
	});

	if (!tookLock) loadLocker.waitForUnlock();
}

inline void logException(const exception& ex, const string& msg, bool fatal) {
	Locker& errLocker = _deps->getErrLocker();

	bool tookLock = errLocker.tryLock([&ex, &msg, fatal]() {
		string fullMsg = msg + "\nException:\n" + string(ex.what());

		if (_deps == nullptr) {
			showErrorMessage(fullMsg, _moduleName);
			return;
		}

		Logger& logger = _deps->getLogger();

		if (fatal) {
			logger.logFatal(fullMsg);
			_deps->getMsgBoxLogger().logFatal(fullMsg);
		}
		else {
			logger.logError(fullMsg);
		}
	});

	if (!tookLock) errLocker.waitForUnlock();
}

inline void createDirectory(const string& dirPath) {
	WinApiDirectoryCreator dirCreator;
	dirCreator.createDir(dirPath);
}


inline void initializeComponents(const HMODULE& handle) {
	_moduleName = getModuleName(handle);
	_deps = new DefaultExtensionDepsContainer(_moduleName);
	createDirectory(getConfig(true).logDirPath);
}

inline void freeComponents() {
	if(_deps != nullptr) delete _deps;
	_deps = nullptr;
	_disabledByErr.store(false);
	_moduleName = "";
}


BOOL WINAPI DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		try {
			initializeComponents(hModule);
			ExtensionConfig config = getConfig(true);
			
			if (!config.scriptPath.empty()) {
				ScriptManager& scriptManager = getScriptManager();
				loadScript(scriptManager, config, false, 
					"Initializing python script: " + config.scriptPath, "Python script loaded and initialized: " + config.scriptPath);
				
				if(!scriptManager.isScriptLoaded()) {
					logException(runtime_error("None..."), "Failed to initialize python script. Disabling python extension functionality. Check logs for more details.", true);
					_disabledByErr.store(true);
				}
			}
		}
		catch (exception& ex) {
			logException(ex, "Failed to initialize extension. Check logs for more details.", true);
			freeComponents();
			throw;
		}
		break;
	case DLL_PROCESS_DETACH:
		try {
			_deps->getLogger().logInfo("Detaching extension: " + _moduleName);
			getScriptManager().unloadPythonScript();
			freeComponents();
		}
		catch (exception& ex) {
			logException(ex, "Failed to run all extension detachment tasks. Check logs for more details.", true);
			freeComponents();
			throw;
		}
		break;
	}

	return TRUE;
}


/*
	Param sentence: sentence received by Textractor (UTF-16). Can be modified, Textractor will receive this modification only if true is returned.
	Param sentenceInfo: contains miscellaneous info about the sentence (see README).
	Return value: whether the sentence was modified.
	Textractor will display the sentence after all extensions have had a chance to process and/or modify it.
	The sentence will be destroyed if it is empty or if you call Skip().
	This function may be run concurrently with itself: please make sure it's thread safe.
	It will not be run concurrently with DllMain.
*/
bool ProcessSentence(std::wstring& sentence, SentenceInfo sentenceInfo)
{
	SentenceInfoWrapper sentInfoWrapper(sentenceInfo);

	try {
		ScriptManager& scriptManager = getScriptManager();
		return ProcessSentenceBase(scriptManager, sentence, sentInfoWrapper);
	}
	catch (exception& ex) {
		try {
			logException(ex, "An unhandled exception has occurred. Will attempt to reload script. Check logs for more details.", false);
			
			ScriptManager& scriptManager = getScriptManager();
			ExtensionConfig config = getConfig(false);

			loadScript(scriptManager, config, true,
				"Attempting to reload python script due to an unhandled exception. Check logs for more details. Script: " + config.scriptPath,
				"Python script reload successful. Script: " + config.scriptPath
			);

			return ProcessSentenceBase(scriptManager, sentence, sentInfoWrapper);
		}
		catch (exception& ex2) {
			logException(ex2, "Another unhandled exception occurred after attempting to reload script. Disabling python script. Check logs for more details.", true);
			_disabledByErr.store(true);
		}
	}

	return false;
}

bool ProcessSentenceBase(ScriptManager& scriptManager, wstring& sentence, SentenceInfoWrapper& sentInfoWrapper) {
	ExtensionConfig config = getConfig(false);
	if (!meetsExecutionRequirements(sentInfoWrapper, config)) return false;

	_deps->getConfigAdjustEvents().applyConfigAdjustments(config);
	if (config.scriptPath.empty()) return false;
	if (!validScriptState(scriptManager)) return false;

	sentence = scriptManager.processSentenceFromScript(sentence, sentInfoWrapper, config.appendErrMsg);
	return true;
}

bool meetsExecutionRequirements(SentenceInfoWrapper& sentInfoWrapper, const ExtensionConfig& config) {
	if (config.disabled) return false;
	if (config.activeThreadOnly && !sentInfoWrapper.isActiveThread()) return false;
	if (!meetsConsoleAndClipboardRequirements(sentInfoWrapper, config)) return false;

	return true;
}

bool meetsConsoleAndClipboardRequirements(SentenceInfoWrapper& sentInfoWrapper, const ExtensionConfig& config) {
	string threadName = sentInfoWrapper.getThreadName();

	switch (config.skipConsoleAndClipboard) {
	case 1:
		if (sentInfoWrapper.threadIsConsoleOrClipboard()) return false;
		break;
	case 2:
		if (threadName == "Console") return false;
		break;
	case 3:
		if (threadName == "Clipboard") return false;
		break;
	}

	return true;
}

bool validScriptState(const ScriptManager& scriptManager) {
	bool scriptLoaded = scriptManager.isScriptLoaded();
	string currScriptPath = scriptManager.getScriptPath();

	if (_disabledByErr.load()) {
		if (scriptLoaded) {
			_disabledByErr.store(false);
			return true;
		}

		return false;
	}
	else if (!scriptLoaded) {
		throw runtime_error("Script is unexpectedly no longer loaded. Check logs for more details.");
	}

	return true;
}
