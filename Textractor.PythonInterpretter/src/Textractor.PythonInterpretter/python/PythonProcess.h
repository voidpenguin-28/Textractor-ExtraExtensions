#pragma once

#include "../Libraries/strhelper.h"
#include "../logging/LoggerBase.h"
#include "pip/PipPackageInstaller.h"
#include "PipeManager.h"
#include "ProcessManager.h"
#include "PythonInitCodeReserve.h"
#include "PythonThread.h"
#include "WinApiHelper.h"
#include "WinEventHandler.h"
#include <functional>
#include <memory>
#include <unordered_map>
using PythonThreadPtr = shared_ptr<PythonThread>;
using PythonThreadMap = unordered_map<string, PythonThreadPtr>;


class PythonProcess {
public:
	virtual ~PythonProcess() { }

	virtual bool isEnabled() const = 0;
	virtual bool isActive() const = 0;
	virtual bool enableConnection(bool resetConnection = false) = 0;
	virtual void disableConnection() = 0;
	virtual PythonThreadPtr createOrGetThread(const string& identifier, const size_t bufferSizeOverride = 0) = 0;
	virtual void closeThread(const string& identifier) = 0;
};

class DefaultPythonProcess : public PythonProcess {
public:
	DefaultPythonProcess(ProcessManager& procManager, const Logger& logger, const WinEventHandler& eventHandler, 
		const PythonInitCodeReserve& pyCodeReserve, PipPackageInstaller& packageInstaller,
		const function<PipeManagerPtr(const string& pipeName, const size_t bufferSize)>& pipeGen,
		const function<int()> showConsoleWindowGetter = []() { return 0; })
		: _rootIdentifier(procManager.Identifier), _procManager(procManager), 
			_logger(logger), _eventHandler(eventHandler), _pyCodeReserve(pyCodeReserve), 
			_packageInstaller(packageInstaller), _pipeGen(pipeGen),
			_showConsoleWindowGetter(showConsoleWindowGetter) { }

	virtual ~DefaultPythonProcess() {
		if (isEnabled()) disableConnection();
	}

	bool isEnabled() const override {
		return _managerThread != nullptr;
	}

	bool isActive() const override {
		return isEnabled() && _procManager.isProcessActive() && _managerThread->isActive();
	}

	bool enableConnection(bool resetConnection = false) override {
		_logger.logInfo("***** Enabling PythonInterpretter. Identifier: " + _rootIdentifier);

		if (isEnabled()) {
			if (resetConnection || !isActive()) disableConnection();
			else return true;
		}

		installRequiredPipPackages();
		
		string mainPipeName = createPipeName(_pyCodeReserve.ManagerPipeId);
		PipeManagerPtr mainPipeManager = _pipeGen(mainPipeName, 0);

		DWORD errCode = mainPipeManager->createPipe(false);
		if (isErr(errCode)) {
			disableConnection(true, true);
			return false;
		}

		_pyCodeReserve.exportInitScripts(_rootIdentifier);
		string command = createPyStartCommand(mainPipeManager->BufferSize);

		errCode = _procManager.createProcess(command, true);
		if (isErr(errCode)) {
			mainPipeManager->closePipe();
			disableConnection(true, true);
			return false;
		}

		if (!_eventHandler.createEventAndWait(mainPipeName, errCode)) {
			mainPipeManager->closePipe();
			disableConnection(true, true);
			return false;
		}

		errCode = mainPipeManager->connectToPipe();
		if (isErr(errCode)) {
			mainPipeManager->closePipe();
			disableConnection(true, true);
			return false;
		}

		_managerThread = createManagerThread(mainPipeManager);
		if (!_managerThread->isActive()) {
			disableConnection();
			return false;
		}

		return true;
	}

	void disableConnection() override {
		disableConnection(false, false);
	}

	PythonThreadPtr createOrGetThread(const string& identifier,
		const size_t bufferSizeOverride = 0) override
	{
		bool enabled = isEnabled();
		if (enabled && !isActive()) disableConnection();
		if (!enabled) return _sharedNullPtr;

		if (threadExists(identifier)) {
			PythonThreadPtr thread = getThread(identifier);
			if (thread->isActive()) return thread;

			thread->close();
		}

		string pipeName = createPipeName(identifier);
		PipeManagerPtr threadPipe = _pipeGen(pipeName, bufferSizeOverride);
		DWORD errCode = threadPipe->createPipe(false);

		if (isErr(errCode)) {
			threadPipe->closePipe();
			return _sharedNullPtr;
		}

		future<pair<bool, DWORD>> eventWait = _eventHandler.createEventAndWaitAsync(pipeName);
		_managerThread->startPiper(identifier, bufferSizeOverride);
		
		if (!eventWait.get().first) {
			threadPipe->closePipe();
			return _sharedNullPtr;
		}

		PythonThreadPtr thread = createPythonThread(identifier, threadPipe);

		if (!thread->isActive()) {
			thread->close();
			return _sharedNullPtr;
		}

		setThread(identifier, thread);
		return thread;
	}

	void closeThread(const string& identifier) override {
		if (isEnabled() && !isActive()) disableConnection();
		if (!isEnabled()) return;

		PythonThreadPtr thread = getThread(identifier);
		thread->close();
	}
private:
	const PythonThreadPtr _sharedNullPtr = PythonThreadPtr(nullptr);

	const string _rootIdentifier;
	ProcessManager& _procManager;
	const Logger& _logger;
	const WinEventHandler& _eventHandler;
	const PythonInitCodeReserve& _pyCodeReserve;
	PipPackageInstaller& _packageInstaller;
	const function<PipeManagerPtr(const string& pipeName, const size_t bufferSize)> _pipeGen;
	const function<int()> _showConsoleWindowGetter;

	unique_ptr<PythonManagerThread> _managerThread = nullptr;
	PythonThreadMap _threadTracker = { };

	bool isErr(DWORD errCode) const {
		return WinApiHelper::isErr(errCode);
	}

	void installRequiredPipPackages() {
		vector<string> reqPackages = _pyCodeReserve.getRequiredPipPackages();
		if (!_packageInstaller.install(reqPackages))
			_logger.logWarning("Failed to install all required pip packages. This will be ignored and python execution will continue. Packages: " + StrHelper::join<char>(", ", reqPackages));
	}

	string createPipeName(const string& identifier) {
		return _rootIdentifier + '-' + identifier;
	}

	string createPyStartCommand(const size_t bufferSize) const {
		PythonInitCodeReserve::CmdPars cmdPars = createPyCmdPars(bufferSize);
		string command = _pyCodeReserve.createStartCmd(_rootIdentifier, cmdPars);
		return command;
	}

	PythonInitCodeReserve::CmdPars createPyCmdPars(const size_t bufferSize) const {
		auto cmdPars = PythonInitCodeReserve::CmdPars(
			_rootIdentifier, bufferSize, _showConsoleWindowGetter(), _logger.getMinLogLevel());

		return cmdPars;
	}

	unique_ptr<PythonManagerThread> createManagerThread(PipeManagerPtr& pipeManager) {
		unique_ptr<PythonManagerThread> baseThread = make_unique<DefaultPythonManagerThread>(
			_pyCodeReserve.ManagerPipeId, move(pipeManager), _procManager, _logger, _pyCodeReserve, false);

		unique_ptr<PythonManagerThread> thread = 
			make_unique<ThreadSafePythonManagerThread>(move(baseThread));

		thread->addToBeforeClose([this](const PythonManagerThread& thr) {
			closeAllThreads();
		});

		thread->addToOnClose([this](const PythonManagerThread& thr) {
			cleanupConnection();
		});

		return thread;
	}

	PythonThreadPtr createPythonThread(
		const string& identifier, PipeManagerPtr& pipeManager)
	{
		unique_ptr<PythonThread> baseThread = make_unique<DefaultPythonThread>(
			identifier, move(pipeManager), _procManager, _logger, _pyCodeReserve);

		PythonThreadPtr thread = make_shared<ThreadSafePythonThread>(move(baseThread));

		thread->addToOnClose([this](const PythonThread& thr) {
			cleanupClosedThread(thr.Identifier);
		});

		return thread;
	}

	bool threadExists(const string& identifier) const {
		return _threadTracker.find(identifier) != _threadTracker.end();
	}

	PythonThreadPtr getThread(const string& identifier) const {
		return threadExists(identifier) ? _threadTracker.at(identifier) : _sharedNullPtr;
	}

	void setThread(const string& identifier, PythonThreadPtr thread) {
		_threadTracker[identifier] = thread;
	}

	void disableConnection(bool closeThreads, bool cleanup) {
		if(closeThreads) closeAllThreads();

		if (_managerThread != nullptr) {
			_managerThread->close();
			_managerThread = nullptr;
		}

		if (cleanup) cleanupConnection();
	}

	void cleanupConnection() {
		if (_procManager.isProcessEnabled()) {
			_procManager.waitForProcessExit();
			_procManager.closeProcess();
			_procManager.waitForProcessExit();
		}

		_pyCodeReserve.removeInitScripts(_rootIdentifier);
	}

	void cleanupClosedThread(const string& identifier) {
		PythonThreadPtr thread = getThread(identifier);
		cleanupClosedThread(thread);
	}

	void cleanupClosedThread(PythonThreadPtr thread) {
		if (thread == nullptr) return;
		_threadTracker.erase(thread->Identifier);
	}

	void closeAllThreads() {
		for (auto& thread : PythonThreadMap(_threadTracker)) {
			thread.second->close();
		}
	}
};
