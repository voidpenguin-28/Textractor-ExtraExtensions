
#pragma once
#include "../environment/EnvironmentVarRetriever.h"
#include "../logging/Loggers.h"
#include "../python/PythonProcess.h"
#include "../ExtensionConfig.h"
#include "LoggerFactory.h"
#include <functional>
#include <memory>
using namespace std;


class PythonContainer {
public:
	virtual ~PythonContainer() { }
	virtual Logger& getLogger() = 0;
	virtual PipPackageInstaller& getPackageInstaller() = 0;
	virtual PythonProcess& getPythonProcess() = 0;
};


class DefaultPythonContainer : public PythonContainer {
public:
	DefaultPythonContainer(const string& moduleName, ConfigRetriever& configRetriever) {
		const ExtensionConfig config = configRetriever.getConfig();
		const string logFilePath = config.logDirPath + moduleName + "-host-log.txt";
		
		_loggerEvents = make_unique<DynamicLoggerEvents>();
		_loggerFactory = make_unique<PersistentWinApiFileWriterLoggerManager>();
		_logger = unique_ptr<Logger>(
			_loggerFactory->createLogger(logFilePath, *_loggerEvents, config.logLevel));

		_stateTracker = make_unique<WinApiProcessStateTracker>();
		_rootProcManager = make_unique<WinApiProcessManager>(moduleName, *_logger, *_stateTracker);
		_mainProcManager = make_unique<ThreadSafeProcessManager>(*_rootProcManager);

		_initCodeReserve = make_unique<DefaultPythonInitCodeReserve>(
			_scriptDirPath, config.logDirPath, config.customPythonPath);

		_eventHandler = make_unique<WinApiWinEventHandler>(*_logger, *_stateTracker);

		_pipProcManager = make_unique<WinApiProcessManager>(moduleName + "-Pip", *_logger, *_stateTracker);
		_pipCmdStrBuilder = make_unique<DefaultPipCmdStrBuilder>(
			config.pipPackageInstallMode, config.customPythonPath);
		_packageInstaller = make_unique<ProcManagerPipPackageInstaller>(
			*_logger, *_pipProcManager, *_pipCmdStrBuilder);

		function<unique_ptr<PipeManager>(const string&, const size_t)> pipeGen =
			[this](const string& pipeName, const size_t bufferSize)
			{
				Logger& logger = getLogger();

				unique_ptr<PipeManager> mainManager = bufferSize > 0 ?
					make_unique<WinApiPipeManager>(logger, pipeName, bufferSize) :
					make_unique<WinApiPipeManager>(logger, pipeName);

				return make_unique<ThreadSafePipeManager>(move(mainManager));
			};

		_pyProc = make_unique<DefaultPythonProcess>(*_mainProcManager, 
			*_logger, *_eventHandler, *_initCodeReserve, *_packageInstaller, pipeGen,
			[&configRetriever]() { return configRetriever.getConfig(false).showLogConsole; }
		);

		_loggerEvents->addToOnLogLevelChangedEvent(
			[moduleName, this](const Logger& logger, Logger::Level prevLevel, Logger::Level currLevel)
			{
				static const string threadName = "Logger-LevelChange";
				if (prevLevel == currLevel) return;
				if (!_pyProc->isEnabled()) return;

				PythonThreadPtr thread = _pyProc->createOrGetThread(threadName);

				if (thread == nullptr) {
					logger.logError("Could not change python log level due to being unable to create Python thread: " + threadName);
					return;
				}

				string setLogLevelCmd = _initCodeReserve->createSetLogLevelCmd(moduleName, currLevel);
				thread->runCommand(setLogLevelCmd);
				_pyProc->closeThread(thread->Identifier);
			});
	}

	Logger& getLogger() override {
		return *_logger;
	}

	PipPackageInstaller& getPackageInstaller() override {
		return *_packageInstaller;
	}

	PythonProcess& getPythonProcess() override {
		return *_pyProc;
	}
private:
	const string _scriptDirPath = getTempPath();

	unique_ptr<PythonProcess> _pyProc = nullptr;

	unique_ptr<DynamicLoggerEvents> _loggerEvents = nullptr;
	unique_ptr<LoggerFactory> _loggerFactory = nullptr;
	unique_ptr<Logger> _logger = nullptr;

	unique_ptr<ProcessStateTracker> _stateTracker = nullptr;
	unique_ptr<ProcessManager> _rootProcManager = nullptr;
	unique_ptr<ProcessManager> _mainProcManager = nullptr;
	unique_ptr<PythonInitCodeReserve> _initCodeReserve = nullptr;
	unique_ptr<WinEventHandler> _eventHandler = nullptr;
	unique_ptr<PipCmdStrBuilder> _pipCmdStrBuilder = nullptr;
	unique_ptr<ProcessManager> _pipProcManager = nullptr;
	unique_ptr<PipPackageInstaller> _packageInstaller = nullptr;

	static string getTempPath() {
		WinStdLibEnvironmentVarRetriever envVarRetriever;
		string tempDirVarName = envVarRetriever.getTempDirVarName();
		return envVarRetriever.getEnvVar(tempDirVarName);
	}
};
