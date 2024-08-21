
#pragma once
#include "_Libraries/datetime.h"
#include "_Libraries/Locker.h"
#include <fstream>
#include <iostream>
#include <string>
using namespace std;

class Logger {
public:
	enum Level { Debug, Info, Warning, Error, Fatal };
	Logger(const Level minLogLevel = Level::Debug) : _minLogLevel(minLogLevel) { }
	virtual ~Logger() { }

	virtual void logDebug(const string& msg) {
		log(Level::Debug, msg);
	}
	virtual void logInfo(const string& msg) {
		log(Level::Info, msg);
	}
	virtual void logWarning(const string& msg) {
		log(Level::Warning, msg);
	}
	virtual void logError(const string& msg) {
		log(Level::Error, msg);
	}
	virtual void logFatal(const string& msg) {
		log(Level::Fatal, msg);
	}

	virtual void log(const Level logLevel, const string& msg) const {
		if (logLevel < _minLogLevel) return;

		string fullMsg = createLogMsg(logLevel, msg);
		writeToLog(fullMsg);
	}

	virtual void setMinLogLevel(const Level minLogLevel) {
		_minLogLevel = minLogLevel;
	}
protected:
	Level _minLogLevel;

	virtual void writeToLog(const string& msg) const = 0;

	virtual string createLogMsg(const Level logLevel, const string& baseMsg) const {
		string levelStr = toStr(logLevel);
		string msg = "[" + getCurrentDateTime() + "] [" + levelStr + "] " + baseMsg;
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

class NoLogger : public Logger {
public:
	void logDebug(const string& msg) const { }
	void logInfo(const string& msg) const { }
	void logWarning(const string& msg) const { }
	void logError(const string& msg) const { }
	void logFatal(const string& msg) const { }
	void log(const Level level, const string& msg) const { }
};


class FileLogger : public Logger {
public:
	FileLogger(const string& logFilePath, Level minLogLevel = Level::Debug)
		: Logger(minLogLevel), _logFilePath(logFilePath) { }

protected:
	string _logFilePath;
	mutable BasicLocker _locker;

	void writeToLog(const string& msg) const {
		_locker.lock([this, &msg]() {
			ofstream file(_logFilePath, ios_base::app);
			file << msg << endl;
			file.close();
		});
	}
};


class COutLogger : public Logger {
public:
	COutLogger(Level minLogLevel = Level::Debug) : Logger(minLogLevel) { }

protected:
	void writeToLog(const string& msg) const {
		cout << msg << endl;
	}
};