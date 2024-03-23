#pragma once

#include "LoggerBase.h"
#include "LoggerEvents.h"
#include <functional>
#include <mutex>
#include <vector>

class NoLoggerEvents : public Logger::Events {
public:
	void onLogLevelChanged(const Logger& logger, 
		Logger::Level prevLogLevel, Logger::Level currLogLevel) const override { }
};

class StaticLoggerEvents : public Logger::Events {
public:
	StaticLoggerEvents(const function<void(const Logger&, Logger::Level, Logger::Level)> onLogLevelChangedFunc_)
		: StaticLoggerEvents(vector<function<void(const Logger&, Logger::Level, Logger::Level)>>{onLogLevelChangedFunc_}) { }
	StaticLoggerEvents(const vector<function<void(const Logger&, Logger::Level, Logger::Level)>> onLogLevelChangedFuncs_)
		: _onLogLevelChangedFuncs(onLogLevelChangedFuncs_) { }

	void onLogLevelChanged(const Logger& logger,
		Logger::Level prevLogLevel, Logger::Level currLogLevel) const override
	{
		for (const auto& func : _onLogLevelChangedFuncs) {
			func(logger, prevLogLevel, currLogLevel);
		}
	}
private:
	const vector<function<void(const Logger&, Logger::Level, Logger::Level)>> _onLogLevelChangedFuncs;
};

class DynamicLoggerEvents : public Logger::Events {
public:
	void onLogLevelChanged(const Logger& logger,
		Logger::Level prevLogLevel, Logger::Level currLogLevel) const override
	{
		lock_guard<mutex> lock(_mutex);
		for (auto& func : _onLogLevelChangedFuncs) {
			func(logger, prevLogLevel, currLogLevel);
		}
	}

	void addToOnLogLevelChangedEvent(function<void(const Logger&, Logger::Level, Logger::Level)> func) {
		lock_guard<mutex> lock(_mutex);
		_onLogLevelChangedFuncs.push_back(func);
	}
private:
	vector<function<void(const Logger&, Logger::Level, Logger::Level)>> _onLogLevelChangedFuncs;
	mutable mutex _mutex;
};
