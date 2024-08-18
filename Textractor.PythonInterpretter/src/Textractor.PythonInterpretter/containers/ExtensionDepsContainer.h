
#pragma once
#include "PythonContainer.h"
#include "../Libraries/Locker.h"
#include "../Libraries/winmsg.h"
#include "../logging/Loggers.h"
#include "../environment/DirectoryCreator.h"
#include "../KeyValSplitter.h"
#include "../Extension.h"
#include "../ExtensionConfig.h"
#include "../ScriptCmdStrHandler.h"
#include "../ConfigAdjustmentEvents.h"
#include "../ExtExecRequirements.h"
#include "../ScriptManager.h"

class ExtensionDepsContainer {
public:
	virtual ~ExtensionDepsContainer() { }
	virtual string moduleName() = 0;
	virtual DirectoryCreator& getDirCreator() = 0;
	virtual ScriptManager& getScriptManager() = 0;
	virtual Locker& getLoadLocker() = 0;
	virtual Locker& getErrLocker() = 0;
	virtual Logger& getLogger() = 0;
	virtual Logger& getMsgBoxLogger() = 0;
	virtual ConfigRetriever& getConfigRetriever() = 0;
	virtual ConfigAdjustmentEvents& getConfigAdjustEvents() = 0;
	virtual ExtExecRequirements& getExtExecRequirements() = 0;
};


class DefaultExtensionDepsContainer : public ExtensionDepsContainer {
public:
	DefaultExtensionDepsContainer(const HMODULE& handle) {
		_moduleName = getModuleName(handle);
		_fileTracker = make_unique<WinApiFileTracker>();

		_baseConfigRetriever = make_unique<IniConfigRetriever>(_iniFileName, StrHelper::convertToW(_moduleName));
		_mainConfigRetriever = make_unique<FileWatchMemCacheConfigRetriever>(
			*_baseConfigRetriever, *_fileTracker, _iniFileName);
		ExtensionConfig config = getConfig(true); // set default config if config not defined
		
		const string logFilePath = config.logDirPath + _moduleName + "-extension-log.txt";
		_loggerEvents = make_unique<NoLoggerEvents>();
		_msgBoxLogger = make_unique<WinMsgBoxLogger>(_moduleName, *_loggerEvents, config.logLevel);
		
		
		_loggerFactory = make_unique<PersistentWinApiFileWriterLoggerManager>();
		_mainLogger = unique_ptr<Logger>(
			_loggerFactory->createLogger(logFilePath, *_loggerEvents, config.logLevel));

		_loadLocker = make_unique<BasicLocker>();
		_errLocker = make_unique<BasicLocker>();
		_cmdStrHandler = make_unique<DefaultScriptCmdStrHandler>(
			[this]() { return getConfig().pipRequirementsTxtPath; },
			[this]() {
				ExtensionConfig config = getConfig();
				return _keyValSplitter.splitToKeyVals(config.scriptCustomVars, config.scriptCustomVarsDelim);
			}
		);

		_idGenerator = make_unique<DefaultThreadIdGenerator>();
		_pyContainer = make_unique<DefaultPythonContainer>(_moduleName, *_mainConfigRetriever);

		_baseScriptManager = make_unique<DefaultScriptManager>(
			_pyContainer->getPythonProcess(), *_cmdStrHandler, 
			*_idGenerator, _pyContainer->getPackageInstaller(), *_mainLogger
		);

		_mainScriptManager = make_unique<ThreadSafeScriptManager>(*_baseScriptManager, *_idGenerator);

		vector<reference_wrapper<Logger>> loggers = { *_mainLogger, *_msgBoxLogger, _pyContainer->getLogger() };

		_baseConfigAdjustEvents = make_unique<DefaultConfigAdjustmentEvents>(
			_moduleName, config, *_mainScriptManager, *_fileTracker, *_msgBoxLogger, loggers);
		_mainConfigAdjustEvents = make_unique<CooldownConfigAdjustmentEvents>(*_baseConfigAdjustEvents, 1000);
		_execRequirements = make_unique<DefaultExtExecRequirements>();
		_dirCreator = make_unique<WinApiDirectoryCreator>();
	}

	string moduleName() override {
		return _moduleName;
	}

	DirectoryCreator& getDirCreator() override {
		return *_dirCreator;
	}

	ScriptManager& getScriptManager() override {
		return *_mainScriptManager;
	}

	Locker& getLoadLocker() override {
		return *_loadLocker;
	}

	Locker& getErrLocker() override {
		return *_errLocker;
	}

	Logger& getLogger() override {
		return *_mainLogger;
	}

	Logger& getMsgBoxLogger() override {
		return *_msgBoxLogger;
	}

	ConfigRetriever& getConfigRetriever() override {
		return *_mainConfigRetriever;
	}

	ConfigAdjustmentEvents& getConfigAdjustEvents() override {
		return *_mainConfigAdjustEvents;
	}

	ExtExecRequirements& getExtExecRequirements() override {
		return *_execRequirements;
	}

private:
	const string _iniFileName = "Textractor.ini";
	const long long _logSizeLimitBytes = 10 * 1024 * 1024;
	DefaultKeyValSplitter _keyValSplitter;

	string _moduleName;
	unique_ptr<ScriptManager> _mainScriptManager = nullptr;
	unique_ptr<Locker> _loadLocker = nullptr;
	unique_ptr<Locker> _errLocker = nullptr;
	unique_ptr<LoggerFactory> _loggerFactory = nullptr;
	unique_ptr<Logger> _mainLogger = nullptr;
	unique_ptr<Logger> _msgBoxLogger = nullptr;
	unique_ptr<ConfigRetriever> _mainConfigRetriever = nullptr;
	unique_ptr<ConfigAdjustmentEvents> _mainConfigAdjustEvents = nullptr;
	unique_ptr<ExtExecRequirements> _execRequirements = nullptr;
	unique_ptr<DirectoryCreator> _dirCreator = nullptr;

	unique_ptr<ConfigRetriever> _baseConfigRetriever = nullptr;
	unique_ptr<Logger::Events> _loggerEvents = nullptr;
	unique_ptr<ScriptCmdStrHandler> _cmdStrHandler = nullptr;
	unique_ptr<ThreadIdGenerator> _idGenerator = nullptr;
	unique_ptr<PythonContainer> _pyContainer = nullptr;
	unique_ptr<ScriptManager> _baseScriptManager = nullptr;
	unique_ptr<FileTracker> _fileTracker = nullptr;
	unique_ptr<ConfigAdjustmentEvents> _baseConfigAdjustEvents = nullptr;

	ExtensionConfig getConfig(bool saveDefault = false) {
		return _mainConfigRetriever->getConfig(saveDefault);
	}
};
