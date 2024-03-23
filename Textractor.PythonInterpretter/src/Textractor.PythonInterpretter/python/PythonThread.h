#pragma once
#include "PipeManager.h"
#include "ProcessManager.h"
#include "PythonInitCodeReserve.h"
#include <functional>
#include <vector>
using namespace std;
using PipeManagerPtr = unique_ptr<PipeManager>;


class PythonThreadBase {
public:
	PythonThreadBase(const string& identifier) : Identifier(identifier) { }
	virtual ~PythonThreadBase() { }
	const string Identifier;

	virtual bool isActive() const = 0;
	virtual void close() = 0;
};

class PythonThread : public PythonThreadBase {
public:
	PythonThread(const string& identifier) : PythonThreadBase(identifier) { }
	virtual ~PythonThread() { }
	virtual void addToBeforeClose(const function<void(const PythonThread& thread)> beforeCloseAction) = 0;
	virtual void addToOnClose(const function<void(const PythonThread& thread)> onCloseAction) = 0;
	virtual string runCommand(const string& command) = 0;
protected:
	vector<function<void(const PythonThread& thread)>> _beforeCloseActions;
	vector<function<void(const PythonThread& thread)>> _onCloseActions;
};

class PythonManagerThread : public PythonThreadBase {
public:
	PythonManagerThread(const string& identifier) : PythonThreadBase(identifier) { }
	virtual ~PythonManagerThread() { }
	virtual void addToBeforeClose(const function<void(const PythonManagerThread& thread)> beforeCloseAction) = 0;
	virtual void addToOnClose(const function<void(const PythonManagerThread& thread)> onCloseAction) = 0;
	virtual void startPiper(const string& identifier, const size_t bufferSizeOverride = 0) = 0;
protected:
	vector<function<void(const PythonManagerThread& thread)>> _beforeCloseActions;
	vector<function<void(const PythonManagerThread& thread)>> _onCloseActions;
};


class DefaultPythonThreadBase : public PythonThreadBase {
public:
	DefaultPythonThreadBase(const string& identifier, PipeManagerPtr pipeManager,
		const ProcessManager& procManager, const Logger& logger,
		const PythonInitCodeReserve& pyCodeReserve, bool connectPipe = true)
		: PythonThreadBase(identifier), _pipeManager(move(pipeManager)),
		_procManager(procManager), _logger(logger), _pyCodeReserve(pyCodeReserve)
	{
		if (connectPipe) {
			DWORD errCode = _pipeManager->connectToPipe();
			if (isErr(errCode)) close();
		}
	}

	virtual ~DefaultPythonThreadBase() override {
		if(isEnabled()) close();
	}

	virtual bool isActive() const override {
		return _procManager.isProcessActive() && _pipeManager->isPipeActive();
	}

	virtual void close() override {
		disconnectPythonPipe();
		if(_pipeManager->isPipeEnabled()) _pipeManager->closePipe();
	}
protected:
	static constexpr int CMD_NUM_RETRIES = 3;

	PipeManagerPtr _pipeManager;
	const ProcessManager& _procManager;
	const Logger& _logger;
	const PythonInitCodeReserve& _pyCodeReserve;


	virtual bool isErr(DWORD errCode) const {
		return WinApiHelper::isErr(errCode);
	}

	virtual bool isEnabled() const {
		return _pipeManager->isPipeEnabled() && _procManager.isProcessEnabled();
	}

	virtual string runCommand_(const string& command, int numRetries, bool attemptRecovery) {
		if (!isEnabled() || !isActive()) return "";
		if (numRetries < 0) return "";

		DWORD errCode = _pipeManager->writeToPipe(command);
		string output = "";

		if (!isErr(errCode))
			errCode = _pipeManager->readFromPipe(output);

		if (attemptRecovery && isErr(errCode)) {
			recoverFromErrCode(errCode);
			if (output.empty()) output = runCommand_(command, numRetries - 1, attemptRecovery);
		}

		return parsePipeOutput(output);
	}

	virtual string parsePipeOutput(string output) const {
		return output != _pyCodeReserve.PyNoData ? output : "";
	}

	virtual void recoverFromErrCode(DWORD errCode) {
		DWORD e = errCode;
		if (!isErr(e)) return;
		string errMsg = "";

		if (!_procManager.isProcessActive()) {
			errMsg = "Python process is no longer active. Pipe '" + _pipeManager->PipeName + "'. Connection closed.";
		}
		else if (e == ERROR_INVALID_HANDLE || e == ERROR_BROKEN_PIPE || e == ERROR_BAD_PIPE || e == ERROR_PIPE_NOT_CONNECTED) {
			_logger.logWarning("Pipe not available. Attempting to reset pipe '"
				+ _pipeManager->PipeName + "'. ErrCode: " + to_string(errCode));

			DWORD resetErrCode;
			if (!resetPipe(true, resetErrCode) || isErr(resetErrCode)) {
				errMsg = "Failed to reset pipe '" + _pipeManager->PipeName
					+ "'. Connection Closed. ErrCode Before Reset Attempt: " + to_string(e)
					+ "; ErrCode After Reset Attempt: " + to_string(resetErrCode);
			}
		}
		else {
			errMsg = "An unhandled exception has occurred. Pipe '" + _pipeManager->PipeName
				+ "'. Connection closed. ErrCode: " + to_string(errCode);
		}

		if (!errMsg.empty()) {
			close();
			_logger.logError(errMsg);
			throw runtime_error(errMsg);
		}
	}

	virtual bool resetPipe(bool connectPipe, DWORD& errCode) {
		_pipeManager->closePipe();
		errCode = _pipeManager->createPipe(false);

		if (isErr(errCode)) return false;
		if (!connectPipe) return true;

		if (!_procManager.isProcessActive(200)) {
			_pipeManager->closePipe();
			return false;
		}

		errCode = _pipeManager->connectToPipe();
		if (isErr(errCode)) {
			_pipeManager->closePipe();
			return false;
		}

		return true;
	}

	virtual void disconnectPythonPipe() {
		if (!isEnabled()) return;
		if (!_pipeManager->isPipeActive()) return;

		string command = getStopCommand();
		runCommand_(command, 0, false);
	}

	virtual string getStopCommand() const {
		return _pyCodeReserve.StopCmd;
	}
};

class DefaultPythonManagerThread : public PythonManagerThread, public DefaultPythonThreadBase {
public:
	DefaultPythonManagerThread(const string& identifier, PipeManagerPtr pipeManager,
		const ProcessManager& procManager, const Logger& logger,
		const PythonInitCodeReserve& pyCodeReserve, bool connectPipe = true)
		: DefaultPythonThreadBase(identifier, move(pipeManager), procManager,
			logger, pyCodeReserve, connectPipe), PythonManagerThread(identifier) { }

	virtual ~DefaultPythonManagerThread() { }

	virtual bool isActive() const override {
		return DefaultPythonThreadBase::isActive();
	}

	virtual void close() override {
		beforeClose(*this);
		DefaultPythonThreadBase::close();
		onClose(*this);
	}

	void addToBeforeClose(const function<void(const PythonManagerThread& thread)> beforeCloseAction) override {
		_beforeCloseActions.push_back(beforeCloseAction);
	}

	void addToOnClose(const function<void(const PythonManagerThread& thread)> onCloseAction) override {
		_onCloseActions.push_back(onCloseAction);
	}

	void startPiper(const string& identifier, const size_t bufferSizeOverride = 0) override {
		string command = _pyCodeReserve.createCmdPiperStartCmd(identifier, bufferSizeOverride);
		runCommand(command);
	}
protected:
	virtual void beforeClose(const PythonManagerThread& thread) {
		for (const auto& action : _beforeCloseActions) {
			action(thread);
		}
	}

	virtual void onClose(const PythonManagerThread& thread) {
		for (const auto& action : _onCloseActions) {
			action(thread);
		}
	}

	virtual void runCommand(const string& command) {
		runCommand_(command, CMD_NUM_RETRIES, true);
	}
};

class DefaultPythonThread : public PythonThread, public DefaultPythonThreadBase {
public:
	DefaultPythonThread(const string& identifier, PipeManagerPtr pipeManager,
		const ProcessManager& procManager, const Logger& logger,
		const PythonInitCodeReserve& pyCodeReserve, bool connectPipe = true)
		: DefaultPythonThreadBase(identifier, move(pipeManager), procManager,
			logger, pyCodeReserve, connectPipe), PythonThread(identifier) { }

	virtual ~DefaultPythonThread() { }
	
	virtual bool isActive() const override {
		return DefaultPythonThreadBase::isActive();
	}

	virtual void close() override {
		beforeClose(*this);
		DefaultPythonThreadBase::close();
		onClose(*this);
	}

	void addToBeforeClose(const function<void(const PythonThread& thread)> beforeCloseAction) override {
		_beforeCloseActions.push_back(beforeCloseAction);
	}

	void addToOnClose(const function<void(const PythonThread& thread)> onCloseAction) override {
		_onCloseActions.push_back(onCloseAction);
	}

	string runCommand(const string& command) override {
		return runCommand_(command, CMD_NUM_RETRIES, true);
	}
protected:
	virtual void beforeClose(const PythonThread& thread) {
		for (const auto& action : _beforeCloseActions) {
			action(thread);
		}
	}

	virtual void onClose(const PythonThread& thread) {
		for (const auto& action : _onCloseActions) {
			action(thread);
		}
	}
};


class ThreadSafePythonManagerThread : public PythonManagerThread {
public:
	ThreadSafePythonManagerThread(unique_ptr<PythonManagerThread> mainThread) 
		: PythonManagerThread(mainThread->Identifier), _mainThread(move(mainThread)) { }
	
	virtual ~ThreadSafePythonManagerThread() { }
	
	virtual bool isActive() const override {
		bool active = false;
		_coreLocker.waitForUnlock();

		_semaLocker.lock([this, &active]() {
			active = _mainThread->isActive();
		});

		return active;
	}

	virtual void close() override {
		_coreLocker.lock([this]() {
			_semaLocker.waitForAllUnlocked();
			_mainThread->close();
		});
	}

	virtual void addToBeforeClose(const function<void(
		const PythonManagerThread& thread)> beforeCloseAction) override
	{
		_coreLocker.lock([this, &beforeCloseAction]() {
			_mainThread->addToBeforeClose(beforeCloseAction);
		});
	}

	virtual void addToOnClose(const function<void(const PythonManagerThread& thread)> onCloseAction) override {
		_coreLocker.lock([this, &onCloseAction]() {
			_mainThread->addToOnClose(onCloseAction);
		});
	}

	void startPiper(const string& identifier, const size_t bufferSizeOverride = 0) override {
		_coreLocker.lock([this, &identifier, bufferSizeOverride]() {
			_semaLocker.waitForAllUnlocked();
			_mainThread->startPiper(identifier, bufferSizeOverride);
		});
	}
private:
	mutable BasicLocker _coreLocker;
	mutable SemaphoreLocker _semaLocker;
	unique_ptr<PythonManagerThread> _mainThread;
};

class ThreadSafePythonThread : public PythonThread {
public:
	ThreadSafePythonThread(unique_ptr<PythonThread> mainThread)
		: PythonThread(mainThread->Identifier), _mainThread(move(mainThread)) { }

	virtual ~ThreadSafePythonThread() { }

	virtual bool isActive() const override {
		bool active = false;
		_coreLocker.waitForUnlock();

		_semaLocker.lock([this, &active]() {
			active = _mainThread->isActive();
		});

		return active;
	}

	virtual void close() override {
		_coreLocker.lock([this]() {
			_semaLocker.waitForAllUnlocked();
			_mainThread->close();
		});
	}

	virtual void addToBeforeClose(const function<void(
		const PythonThread& thread)> beforeCloseAction) override
	{
		_coreLocker.lock([this, &beforeCloseAction]() {
			_mainThread->addToBeforeClose(beforeCloseAction);
		});
	}

	virtual void addToOnClose(const function<void(const PythonThread& thread)> onCloseAction) override {
		_coreLocker.lock([this, &onCloseAction]() {
			_mainThread->addToOnClose(onCloseAction);
		});
	}

	string runCommand(const string& command) override {
		string output;

		_coreLocker.lock([this, &command, &output]() {
			_semaLocker.waitForAllUnlocked();
			output = _mainThread->runCommand(command);
		});

		return output;
	}
private:
	mutable BasicLocker _coreLocker;
	mutable SemaphoreLocker _semaLocker;
	unique_ptr<PythonThread> _mainThread;
};

