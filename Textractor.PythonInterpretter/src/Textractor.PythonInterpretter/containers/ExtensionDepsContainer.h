
#pragma once
#include "PythonContainer.h"
#include "../environment/FileTracker.h"
#include "../logging/Loggers.h"
#include "../python/Locker.h"
#include "../StringHelper.h"
#include "../Extension.h"
#include "../ExtensionConfig.h"
#include "../ScriptCmdStrHandler.h"
#include "../ConfigAdjustmentEvents.h"
#include "../ScriptManager.h"

class ExtensionDepsContainer {
public:
	virtual ~ExtensionDepsContainer() { }
	virtual ScriptManager& getScriptManager() = 0;
	virtual Locker& getLoadLocker() = 0;
	virtual Locker& getErrLocker() = 0;
	virtual Logger& getLogger() = 0;
	virtual Logger& getMsgBoxLogger() = 0;
	virtual ConfigRetriever& getConfigRetriever() = 0;
	virtual ConfigAdjustmentEvents& getConfigAdjustEvents() = 0;
};


class DefaultExtensionDepsContainer : public ExtensionDepsContainer {
public:
	DefaultExtensionDepsContainer(const string& moduleName) {
		_configRetriever = make_unique<IniConfigRetriever>(_iniFileName, convertToW(moduleName));
		ExtensionConfig config = _configRetriever->getConfig(true); // set default config if config not defined
		
		const string logFilePath = config.logDirPath + moduleName + "-extension-log.txt";
		_loggerEvents = make_unique<NoLoggerEvents>();
		_logger = make_unique<TruncableFileLogger>(logFilePath, _logSizeLimitBytes, *_loggerEvents, config.logLevel);
		_msgBoxLogger = make_unique<WinMsgBoxLogger>(moduleName, *_loggerEvents, config.logLevel);

		_loadLocker = make_unique<BasicLocker>();
		_errLocker = make_unique<BasicLocker>();
		_cmdStrHandler = make_unique<DefaultScriptCmdStrHandler>(
			[this]() { return _configRetriever->getConfig(false).pipRequirementsTxtPath; },
			[this]() {
				ExtensionConfig config = _configRetriever->getConfig(false);
				return _strHelper.splitToKeyVals(config.scriptCustomVars, config.scriptCustomVarsDelim);
			}
		);

		_idGenerator = make_unique<DefaultThreadIdGenerator>();
		_pyContainer = make_unique<DefaultPythonContainer>(moduleName, config);

		_rootScriptManager = make_unique<DefaultScriptManager>(
			_pyContainer->getPythonProcess(), *_cmdStrHandler, 
			*_idGenerator, _pyContainer->getPackageInstaller(), *_logger
		);

		_scriptManager = make_unique<ThreadSafeScriptManager>(*_rootScriptManager, *_idGenerator);

		_fileTracker = make_unique<WinApiFileTracker>();
		vector<reference_wrapper<Logger>> loggers = { *_logger, *_msgBoxLogger, _pyContainer->getLogger()};

		_rootConfigAdjustEvents = make_unique<DefaultConfigAdjustmentEvents>(
			moduleName, *_scriptManager, *_fileTracker, *_msgBoxLogger, loggers);
		_configAdjustEvents = make_unique<CooldownConfigAdjustmentEvents>(*_rootConfigAdjustEvents, 1000);
	}

	ScriptManager& getScriptManager() override {
		return *_scriptManager;
	}

	Locker& getLoadLocker() override {
		return *_loadLocker;
	}

	Locker& getErrLocker() override {
		return *_errLocker;
	}

	Logger& getLogger() override {
		return *_logger;
	}

	Logger& getMsgBoxLogger() override {
		return *_msgBoxLogger;
	}

	ConfigRetriever& getConfigRetriever() override {
		return *_configRetriever;
	}

	ConfigAdjustmentEvents& getConfigAdjustEvents() override {
		return *_configAdjustEvents;
	}

private:
	const string _iniFileName = "Textractor.ini";
	const long long _logSizeLimitBytes = 10 * 1024 * 1024;
	DefaultStringHelper _strHelper;

	unique_ptr<ScriptManager> _scriptManager = nullptr;
	unique_ptr<Locker> _loadLocker = nullptr;
	unique_ptr<Locker> _errLocker = nullptr;
	unique_ptr<Logger> _logger = nullptr;
	unique_ptr<Logger> _msgBoxLogger = nullptr;
	unique_ptr<ConfigRetriever> _configRetriever = nullptr;
	unique_ptr<ConfigAdjustmentEvents> _configAdjustEvents = nullptr;

	unique_ptr<Logger::Events> _loggerEvents = nullptr;
	unique_ptr<ScriptCmdStrHandler> _cmdStrHandler = nullptr;
	unique_ptr<ThreadIdGenerator> _idGenerator = nullptr;
	unique_ptr<PythonContainer> _pyContainer = nullptr;
	unique_ptr<ScriptManager> _rootScriptManager = nullptr;
	unique_ptr<FileTracker> _fileTracker = nullptr;
	unique_ptr<ConfigAdjustmentEvents> _rootConfigAdjustEvents = nullptr;
};
