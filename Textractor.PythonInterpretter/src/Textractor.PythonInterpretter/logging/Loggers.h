#pragma once

#include "../Libraries/winmsg.h"
#include "LoggerBase.h"
#include "LoggerEvents.h"
#include "../File/Writer/FileWriter.h"
#include "../File/FileTruncater.h"
#include <iostream>
#include <vector>
using namespace std;


class NoLogger : public Logger {
public:
	void logDebug(const string& msg) const override { }
	void logInfo(const string& msg) const override { }
	void logWarning(const string& msg) const override { }
	void logError(const string& msg) const override { }
	void logFatal(const string& msg) const override { }
	void log(const Level level, const string& msg) const override { }
	Level getMinLogLevel() const override { return Level::Debug; }
	void setMinLogLevel(const Level minLogLevel) override { }
};


class MultiLogger : Logger {
public:
	MultiLogger(const vector<reference_wrapper<Logger>>& loggers) : _loggers(loggers) { }

	void logDebug(const string& msg) const override {
		log(Level::Debug, msg);
	}
	void logInfo(const string& msg) const override {
		log(Level::Info, msg);
	}
	void logWarning(const string& msg) const override {
		log(Level::Warning, msg);
	}
	void logError(const string& msg) const override {
		log(Level::Error, msg);
	}
	void logFatal(const string& msg) const override {
		log(Level::Fatal, msg);
	}
	void log(const Level level, const string& msg) const override {
		for (const auto& logger : _loggers) {
			logger.get().log(level, msg);
		}
	}
	void setMinLogLevel(const Level minLogLevel) override {
		for (auto& logger : _loggers) {
			logger.get().setMinLogLevel(minLogLevel);
		}
	}
private:
	const vector<reference_wrapper<Logger>> _loggers;
};


class FileWriterLogger : public LoggerBase {
public:
	FileWriterLogger(FileWriter& fileWriter, FileTruncater& fileTruncater, const string& logFilePath, 
		const Logger::Events& events, Level minLogLevel = Level::Debug) : LoggerBase(minLogLevel, events), 
		_logFilePath(logFilePath), _fileWriter(fileWriter), _fileTruncater(fileTruncater) 
	{
		truncateFile();
	}

	~FileWriterLogger() {
		truncateFile();
	}
private:
	FileWriter& _fileWriter;
	FileTruncater& _fileTruncater;
	string _logFilePath;

	void writeToLog(Level logLevel, const string& msg) const override {
		_fileWriter.appendToFile(_logFilePath, msg);
	}

	void truncateFile() {
		_fileTruncater.truncateFile(_logFilePath);
	}
};


class COutLogger : public LoggerBase {
public:
	COutLogger(const Logger::Events& events, Level minLogLevel = Level::Debug)
		: LoggerBase(minLogLevel, events) { }

protected:
	void writeToLog(Level logLevel, const string& msg) const override {
		cout << msg << endl;
	}
};


class WinMsgBoxLogger : public LoggerBase {
public:
	WinMsgBoxLogger(const string& identifier, 
		const Logger::Events& events, Level minLogLevel = Level::Debug)
		: LoggerBase(minLogLevel, events), _identifier(identifier) { }
protected:
	void writeToLog(Level logLevel, const string& msg) const override {
		if (logLevel <= Level::Warning)
			writeToNormalMsgBox(logLevel, msg);
		else
			writeToErrMsgBox(logLevel, msg);
	}

	string createLogMsg(const Level logLevel, const string& baseMsg) const override {
		return baseMsg;
	}
private:
	const string _identifier;

	void writeToNormalMsgBox(Level logLevel, const string& msg) const {
		string header = createHeader(logLevel);
		showTextboxMsg(msg, header);
	}

	void writeToErrMsgBox(Level logLevel, const string& msg) const {
		string header = createHeader(logLevel);
		showErrorMessage(msg, header);
	}

	string createHeader(Level logLevel) const {
		return _identifier + " - " + toStr(logLevel);
	}
};
