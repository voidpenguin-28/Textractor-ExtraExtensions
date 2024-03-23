
#pragma once
#include "Libraries/stringconvert.h"
#include "logging/LoggerBase.h"
#include "python/Locker.h"
#include "python/PythonProcess.h"
#include "Extension.h"
#include "ScriptCmdStrHandler.h"
#include "ThreadIdGenerator.h"
#include <functional>
#include <memory>
#include <mutex>
#include <iostream>
#include <sstream>
#include <string>
using namespace std;
using MutexPtr = shared_ptr<mutex>;


class ScriptManager {
public:
	virtual ~ScriptManager() { }
	virtual string getScriptPath() const = 0;
	virtual bool isScriptLoaded() const = 0;
	virtual bool loadPythonScript(const string& scriptPath, bool resetScript) = 0;
	virtual void unloadPythonScript() = 0;
	virtual wstring processSentenceFromScript(const wstring& sentence, 
		SentenceInfoWrapper& sentenceInfo, bool appendErrMsg = false) = 0;
	virtual string processSentenceFromScript(const string& sentence, 
		SentenceInfoWrapper& sentenceInfo, bool appendErrMsg = false) = 0;
};


class ThreadSafeScriptManager : public ScriptManager {
public:
	ThreadSafeScriptManager(ScriptManager& mainManager, ThreadIdGenerator& idGenerator)
		: _mainManager(mainManager), _idGenerator(idGenerator) { }

	virtual string getScriptPath() const override {
		string scriptPath;
		_mainLocker.waitForUnlock();

		_semaLocker.lock([this, &scriptPath]() {
			scriptPath = _mainManager.getScriptPath();
		});

		return scriptPath;
	}

	virtual bool isScriptLoaded() const override {
		bool loaded = false;
		_mainLocker.waitForUnlock();

		_semaLocker.lock([this, &loaded]() {
			loaded = _mainManager.isScriptLoaded();
		});

		return loaded;
	}

	virtual bool loadPythonScript(const string& scriptPath, bool resetScript) override {
		bool loaded = false;
		bool tookLock = _loadLocker.tryLock([this, &loaded, &scriptPath, resetScript]() {
			_mainLocker.lock([this, &loaded, &scriptPath, resetScript]() {
				_semaLocker.waitForAllUnlocked();
				loaded = _mainManager.loadPythonScript(scriptPath, resetScript);
			});
		});
		
		if (!tookLock) {
			_loadLocker.waitForUnlock();
			loaded = true;
		}

		return loaded;
	}

	virtual void unloadPythonScript() override {
		bool tookLock = _unloadLocker.tryLock([this]() { 
			_mainLocker.lock([this]() {
				_semaLocker.waitForAllUnlocked();
				_mainManager.unloadPythonScript();
			});
		});

		if (!tookLock) _unloadLocker.waitForUnlock();
	}

	virtual wstring processSentenceFromScript(const wstring& sentence, 
		SentenceInfoWrapper& sentenceInfo, bool appendErrMsg = false) override
	{
		MutexPtr threadMutex = getThreadMutex(sentenceInfo);
		lock_guard<mutex> lock(*threadMutex);
		return _mainManager.processSentenceFromScript(sentence, sentenceInfo, appendErrMsg);
	}

	virtual string processSentenceFromScript(const string& sentence, 
		SentenceInfoWrapper& sentenceInfo, bool appendErrMsg = false) override
	{
		MutexPtr threadMutex = getThreadMutex(sentenceInfo);
		lock_guard<mutex> lock(*threadMutex);
		return _mainManager.processSentenceFromScript(sentence, sentenceInfo, appendErrMsg);
	}
private:
	ScriptManager& _mainManager;
	ThreadIdGenerator& _idGenerator;
	mutable BasicLocker _mainLocker;
	mutable BasicLocker _loadLocker;
	mutable BasicLocker _unloadLocker;
	mutable SemaphoreLocker _semaLocker;
	unordered_map<string, MutexPtr> _threadMutexMap;
	
	MutexPtr getThreadMutex(SentenceInfoWrapper& sentenceInfo) {
		string threadId = _idGenerator.generateId(sentenceInfo);
		return getThreadMutex(threadId);
	}

	MutexPtr getThreadMutex(const string& threadId) {
		_mainLocker.waitForUnlock();
		if (!hasThreadMutex(threadId)) setThreadMutex(threadId);
		return _threadMutexMap[threadId];
	}

	bool hasThreadMutex(const string& threadId) const {
		return _threadMutexMap.find(threadId) != _threadMutexMap.end();
	}

	void setThreadMutex(const string& threadId) {
		_threadMutexMap[threadId] = make_shared<mutex>();
	}
};


class DefaultScriptManager : public ScriptManager {
public:
	DefaultScriptManager(PythonProcess& python, ScriptCmdStrHandler& cmdStrHandler, 
		ThreadIdGenerator& idGenerator, PipPackageInstaller& packageInstaller, Logger& logger)
		: _python(python), _cmdStrHandler(cmdStrHandler), _idGenerator(idGenerator), 
			_packageInstaller(packageInstaller), _logger(logger), _currentScriptPath("") { }

	virtual ~DefaultScriptManager() {
		unloadPythonScript();
	}

	virtual  string getScriptPath() const override {
		return _currentScriptPath;
	}

	virtual bool isScriptLoaded() const override {
		return _python.isActive();
	}

	virtual bool loadPythonScript(const string& scriptPath, bool resetScript) override {
		if (!resetScript && isScriptLoaded()) return true;
		installPipPackagesFromReqsTxt();

		if (!_python.enableConnection(resetScript)) {
			_logger.logFatal("Failed to open python terminal. Make sure that python 3.x is installed on your machine.");
			return false;
		}

		PythonThreadPtr loadThread = _python.createOrGetThread(_loadThreadId);
		if (loadThread == nullptr) {
			string pyThreadErrMsg = getPyThreadErrMsg(_loadThreadId);
			_logger.logFatal("Unable to load script. " + pyThreadErrMsg);
			return false;
		}

		bool loadSuccess = true;
		if (!initializeScript(*loadThread, scriptPath)) {
			_logger.logFatal("Failed to initialize script. Check python logs for more details.");
			loadSuccess = false;
		}

		if (loadSuccess && !runOnScriptLoadCommand(*loadThread)) {
			_logger.logWarning("Function 'on_script_load' returned False, thus the script load has been cancelled. Check logs for more details.");
			loadSuccess = false;
		}

		_python.closeThread(loadThread->Identifier);
		if (!loadSuccess) unloadPythonScript();
		return loadSuccess;
	}

	virtual void unloadPythonScript() override {
		if (!isScriptLoaded()) return;
		PythonThreadPtr unloadThread = _python.createOrGetThread(_unloadThreadId);

		if (unloadThread != nullptr) {
			runOnScriptUnloadCommand(*unloadThread);
			_python.closeThread(unloadThread->Identifier);
		}
		else {
			string pyThreadErrMsg = getPyThreadErrMsg(_unloadThreadId);
			_logger.logWarning("Unable to run unload process for script. " + pyThreadErrMsg);
		}

		_python.disableConnection();
		_currentScriptPath = "";
	}

	virtual wstring processSentenceFromScript(const wstring& sentence, 
		SentenceInfoWrapper& sentenceInfo, bool appendErrMsg = false) override
	{
		string utf8Sentence = convertFromW(sentence);
		string outputSentence = processSentenceFromScript(utf8Sentence, sentenceInfo, appendErrMsg);
		return convertToW(outputSentence);
	}

	virtual string processSentenceFromScript(const string& sentence, 
		SentenceInfoWrapper& sentenceInfo, bool appendErrMsg = false) override
	{
		if (!isScriptLoaded()) return sentence;

		string command = _cmdStrHandler.getOnProcessSentenceCommand(sentence, sentenceInfo, appendErrMsg);
		string threadId = _idGenerator.generateId(sentenceInfo);

		string output = runCommand(threadId, command, 
			"Output for 'process_sentence' function call:", true, true, sentence);

		if (_cmdStrHandler.containsErrMsg(output)) 
			output = sentence + ZERO_WIDTH_SPACE + '\n' + output;

		return output;
	}
private:
	const string ZERO_WIDTH_SPACE = convertFromW(L"\x200b");
	const string _loadThreadId = "PyScriptLoad";
	const string _unloadThreadId = "PyScriptUnload";
	string _currentScriptPath;

	PythonProcess& _python;
	ScriptCmdStrHandler& _cmdStrHandler;
	ThreadIdGenerator& _idGenerator;
	PipPackageInstaller& _packageInstaller;
	Logger& _logger;

	string getPyThreadErrMsg(const string& threadId) const {
		static const string errMsg = "Failed to create or get python thread with id: '" + threadId + "'. Check python logs for more details.";
		return errMsg;
	}

	void logPythonOutput(const string& msg, const string& command, 
		const string& output, bool allowDebugMsg = true) const
	{
		Logger::Level logLevel = _logger.getMinLogLevel();

		if(allowDebugMsg && logLevel <= Logger::Debug)
			_logger.logDebug(getDebugOutputMsg(msg, command, output));
		else if(logLevel <= Logger::Info)
			_logger.logInfo(msg + '\n' + output);
	}

	string getDebugOutputMsg(const string& msg, const string& command, const string& output) const {
		return msg + "\nCommand:\n" + command + "\n\nOutput:\n" + output;
	}

	void installPipPackagesFromReqsTxt() {
		string pipReqsTxtPath = _cmdStrHandler.getPipRequirementsTxtPath();
		if (pipReqsTxtPath.empty()) return;

		if (!_packageInstaller.installFromFile(pipReqsTxtPath)) 
			_logger.logWarning("Failed to install all pip packages from requirements txt file. This will be ignored and script execution will continue. ReqsTxtPath: " + pipReqsTxtPath);
	}

	bool initializeScript(PythonThread& py, const string& scriptPath) {
		_logger.logInfo("*** Initializing script: " + scriptPath);
		string script = getScript(scriptPath);
		runCommand(py, script, "Script Initialization Output:", false, false);
		return isProcSentDefined(py);
	}

	bool isProcSentDefined(PythonThread& py) {
		string processSentCheckCmd = _cmdStrHandler.getProcessSentenceDefinedCheckCommand();
		string procSentCheckResult = runCommand(py, processSentCheckCmd, "'process_sentence' definition check:", true, false, "False");
		return procSentCheckResult == "True";
	}

	string getScript(const string& scriptPath) const {
		ifstream file(scriptPath);

		if (!file.is_open()) {
			_logger.logFatal("Error opening script file: " + scriptPath);
			return "";
		}

		ostringstream buffer;
		buffer << file.rdbuf();
		string script = buffer.str();

		file.close();
		return script;
	}

	bool runOnScriptLoadCommand(PythonThread& py) {
		_logger.logInfo("Running 'on_script_load' function on thread: " + py.Identifier);
		string command = _cmdStrHandler.getOnScriptLoadCommand();
		string output = runCommand(py, command, "Output for 'on_script_load' function call:", true, false, "False");
		return output == "True";
	}

	void runOnScriptUnloadCommand(PythonThread& py) {
		_logger.logInfo("Running 'on_script_unload' function on thread: " + py.Identifier);
		string command = _cmdStrHandler.getOnScriptUnloadCommand();
		runCommand(py, command, "Output for 'on_script_unload' function call:", false, false);
	}

	string runCommand(const string& threadId, const string& command,
		const string& logMsg, bool parseOutput, bool allowDebugMsg, const string& outputDefaultValue = "")
	{
		PythonThreadPtr py = _python.createOrGetThread(threadId);

		if (py == nullptr) {
			string pyThreadErrMsg = getPyThreadErrMsg(threadId);
			_logger.logWarning("Unable to run command. " + pyThreadErrMsg + "\nCommand: " + command);
			return "";
		}

		return runCommand(*py, command, logMsg, parseOutput, allowDebugMsg, outputDefaultValue);
	}

	string runCommand(PythonThread& py, const string& command,
		const string& logMsg, bool parseOutput, bool allowDebugMsg, const string& outputDefaultValue = "")
	{
		string output = py.runCommand(command);
		logPythonOutput(logMsg, command, output, allowDebugMsg);

		if (parseOutput) output = _cmdStrHandler.parseCommandOutput(output, outputDefaultValue);
		return output;
	}

};
