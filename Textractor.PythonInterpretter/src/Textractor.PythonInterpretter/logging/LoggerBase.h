#pragma once

#include "../Libraries/datetime.h"
#include <string>
using namespace std;

class Logger {
public:
	enum Level { Debug = 0, Info, Warning, Error, Fatal };
	class Events {
	public:
		~Events() { }
		virtual void onLogLevelChanged(const Logger& logger,
			Logger::Level prevLogLevel, Logger::Level currLogLevel) const = 0;
	};
	
	virtual ~Logger() { }
	virtual void logDebug(const string& msg) const = 0;
	virtual void logInfo(const string& msg) const = 0;
	virtual void logWarning(const string& msg) const = 0;
	virtual void logError(const string& msg) const = 0;
	virtual void logFatal(const string& msg) const = 0;
	virtual void log(const Level logLevel, const string& msg) const = 0;
	virtual Level getMinLogLevel() const = 0;
	virtual void setMinLogLevel(const Level minLogLevel) = 0;
};


class LoggerBase : public Logger {
public:
	LoggerBase(const Logger::Level minLogLevel, const Logger::Events& events) 
		: _minLogLevel(minLogLevel), _events(events) { }
	virtual ~LoggerBase() { }

	virtual void logDebug(const string& msg) const {
		log(Level::Debug, msg);
	}
	virtual void logInfo(const string& msg) const {
		log(Level::Info, msg);
	}
	virtual void logWarning(const string& msg) const {
		log(Level::Warning, msg);
	}
	virtual void logError(const string& msg) const {
		log(Level::Error, msg);
	}
	virtual void logFatal(const string& msg) const {
		log(Level::Fatal, msg);
	}

	virtual void log(const Level logLevel, const string& msg) const {
		if (logLevel < _minLogLevel) return;

		string fullMsg = createLogMsg(logLevel, msg);
		writeToLog(logLevel, fullMsg);
	}

	virtual Level getMinLogLevel() const {
		return _minLogLevel;
	}

	virtual void setMinLogLevel(const Level minLogLevel) {
		if (_minLogLevel == minLogLevel) return;

		Level prevLevel = _minLogLevel;
		_minLogLevel = minLogLevel;
		_events.onLogLevelChanged(*this, prevLevel, minLogLevel);
	}
protected:
	Level _minLogLevel;
	const Events& _events;
	virtual void writeToLog(Level logLevel, const string& msg) const = 0;

	virtual string createLogMsg(const Level logLevel, const string& baseMsg) const {
		string levelStr = toStr(logLevel);
		string msg = "[" + getCurrentDateTime() + "][" + levelStr + "] " + baseMsg;
		return msg;
	}

	virtual string toStr(Level level) const {
		switch (level) {
		case Level::Debug:
			return "DEBUG";
		case Level::Info:
			return "INFO";
		case Level::Warning:
			return "WARN";
		case Level::Error:
			return "ERROR";
		case Level::Fatal:
			return "FATAL";
		default:
			return "DEBUG";
		}
	}
};

