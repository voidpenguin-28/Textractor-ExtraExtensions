
#pragma once
#include "Libraries/FileTracker.h"
#include "Libraries/Locker.h"
#include "logging/LoggerBase.h"
#include "Libraries/winmsg.h"
#include "ExtensionConfig.h"
#include "ScriptManager.h"
#include <chrono>
#include <functional>
#include <vector>
using clock_ = chrono::system_clock;


class ConfigAdjustmentEvents {
public:
	virtual ~ConfigAdjustmentEvents() { }
	virtual void applyConfigAdjustments(const ExtensionConfig& config) = 0;
};


class NoConfigAdjustmentEvents : public ConfigAdjustmentEvents {
	void applyConfigAdjustments(const ExtensionConfig& config) override { }
};

class CooldownConfigAdjustmentEvents : public ConfigAdjustmentEvents {
public:
	CooldownConfigAdjustmentEvents(ConfigAdjustmentEvents& mainAdjust, const size_t cooldownMs) 
		: _mainAdjust(mainAdjust), _cooldownTicks(cooldownMs * MS_TO_TICKS) { }

	virtual void applyConfigAdjustments(const ExtensionConfig& config) override {
		_locker.waitForUnlock();
		if (inCooldown()) return;

		_locker.lock([this, &config]() { 
			_mainAdjust.applyConfigAdjustments(config);
			setLastAdjustTime();
		});
	}
private:
	static constexpr clock_::rep MS_TO_TICKS = 10000;
	BasicLocker _locker;
	ConfigAdjustmentEvents& _mainAdjust;
	const clock_::rep _cooldownTicks;
	atomic<clock_::rep> _lastAdjustTime = 0;

	bool inCooldown() {
		clock_::rep currTime = getCurrentTime();
		clock_::rep timeDiff = currTime - _lastAdjustTime.load();
		return timeDiff < _cooldownTicks;
	}

	clock_::rep getCurrentTime() {
		return clock_::now().time_since_epoch().count();
	}

	void setLastAdjustTime() {
		_lastAdjustTime.store(getCurrentTime());
	}
};


class DefaultConfigAdjustmentEvents : public ConfigAdjustmentEvents {
public:
	DefaultConfigAdjustmentEvents(const string& moduleName, const ExtensionConfig& initConfig, 
		ScriptManager& scriptManager, FileTracker& fileTracker, const Logger& logger, 
		vector<reference_wrapper<Logger>>& trackedLoggers)
		: _moduleName(moduleName), _scriptManager(scriptManager), _fileTracker(fileTracker), 
			_logger(logger), _trackedLoggers(trackedLoggers) 
	{
		_lastScriptPath = initConfig.scriptPath;
		_scriptLastModEpochs = getDateLastModifiedEpochs(initConfig);
		_lastLogLevel = initConfig.logLevel;
	}

	void applyConfigAdjustments(const ExtensionConfig& config) override {
		adjustLogLevels(config);
		reloadScriptIfChanged(config);
		reloadScriptIfModified(config);
	}
private:
	const string _moduleName;
	ScriptManager& _scriptManager;
	FileTracker& _fileTracker;
	const Logger& _logger;
	vector<reference_wrapper<Logger>> _trackedLoggers;
	BasicLocker _locker;

	string _lastScriptPath;
	int64_t _scriptLastModEpochs;
	Logger::Level _lastLogLevel;

	void reloadScriptIfChanged(const ExtensionConfig& config) {
		if (_lastScriptPath == config.scriptPath) return;
		_locker.lock([this, &config]() { reloadScriptDueToChanged(config); });
	}

	void reloadScriptDueToChanged(const ExtensionConfig& config) {
		if (_lastScriptPath == config.scriptPath) return;
		_lastScriptPath = config.scriptPath;

		if (_scriptManager.getScriptPath() != config.scriptPath)
		{
			reloadScript(config);
			_scriptLastModEpochs = getDateLastModifiedEpochs(config);
		}
	}

	void reloadScriptIfModified(const ExtensionConfig& config) {
		if (!config.reloadOnScriptModified) return;
		int64_t currScriptLastModEpochs = getDateLastModifiedEpochs(config);
		if (currScriptLastModEpochs == 0) return;

		_locker.lock([this, &config]() { reloadScriptDueToModified(config); });
	}

	void reloadScriptDueToModified(const ExtensionConfig& config) {
		int64_t currScriptLastModEpochs = getDateLastModifiedEpochs(config);
		if (currScriptLastModEpochs == 0) return;
		if (_scriptLastModEpochs == currScriptLastModEpochs) return;

		_scriptLastModEpochs = currScriptLastModEpochs;
		reloadScript(config);
	}

	int64_t getDateLastModifiedEpochs(const ExtensionConfig& config) {
		return _fileTracker.getDateLastModifiedEpochs(config.scriptPath);
	}

	void reloadScript(const ExtensionConfig& config) {
		_logger.logInfo("Reloading python script due to a detected config or script change. Loading script: " + config.scriptPath);
		_scriptManager.unloadPythonScript();

		if (!config.scriptPath.empty()) {
			_scriptManager.loadPythonScript(config.scriptPath, true);

			if (_scriptManager.isScriptLoaded())
				_logger.logInfo("Python script reloaded successfully.");
			else
				_logger.logError("Python script reload failed. Check logs for more details.");
		}
		else {
			_logger.logInfo("No python script was loaded due to an empty script path.");
		}
	}

	void adjustLogLevelsIfChanged(const ExtensionConfig& config) {
		if (_lastLogLevel == config.logLevel) return;
		_locker.lock([this, &config]() { adjustLogLevelsDueToChanged(config); });
	}

	void adjustLogLevelsDueToChanged(const ExtensionConfig& config) {
		if (_lastLogLevel == config.logLevel) return;
		adjustLogLevels(config);
	}

	void adjustLogLevels(const ExtensionConfig& config) {
		for (auto& logger : _trackedLoggers) {
			adjustLogLevel(logger, config.logLevel);
		}
	}

	void adjustLogLevel(Logger& logger, Logger::Level logLevel) {
		if (logLevel != logger.getMinLogLevel())
			logger.setMinLogLevel(logLevel);
	}
};
