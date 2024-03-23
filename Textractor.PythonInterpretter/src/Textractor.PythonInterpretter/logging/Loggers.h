#pragma once

#include "../Libraries/winmsg.h"
#include "LoggerBase.h"
#include "LoggerEvents.h"
#include <fstream>
#include <iostream>
#include <mutex>
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


class FileLogger : public LoggerBase {
public:
	FileLogger(const string& logFilePath, const Logger::Events& events, Level minLogLevel = Level::Debug)
		: LoggerBase(minLogLevel, events), _logFilePath(logFilePath) { }

	virtual ~FileLogger() { }
protected:
	string _logFilePath;
	mutable mutex _logMutex;

	virtual void writeToLog(Level logLevel, const string& msg) const override {
		lock_guard<mutex> lock(_logMutex);
		ofstream file(_logFilePath, ios_base::app);
		file << msg << endl;
		file.close();
	}
};


class TruncableFileLogger : public FileLogger {
public:
	TruncableFileLogger(const string& logFilePath, const long long sizeLimitBytes,
		const Logger::Events& events, Level minLogLevel = Level::Debug)
		: FileLogger(logFilePath, events, minLogLevel), _sizeLimitBytes(sizeLimitBytes)
	{
		truncateLog();
	}

	virtual ~TruncableFileLogger() {
		truncateLog();
	}

	virtual void truncateLog() {
		lock_guard<mutex> lock(_mutex);
		truncateFile();
	}
protected:
	mutex _mutex;
	const long long _sizeLimitBytes;

	void truncateFile() {
		long long fileSizeBytes = getFileSizeBytes();
		if (fileSizeBytes < _sizeLimitBytes) return;

		double sizeRatio = getSizeRatio(fileSizeBytes);
		pair<vector<string>, long> contents = readAllLines();
		vector<string> lines = contents.first;
		
		size_t startIndex = getTruncStartIndex(lines, contents.second, sizeRatio);
		writeToFile(lines, startIndex);
	}

	long long getFileSizeBytes() {
		ifstream in(_logFilePath, std::ifstream::ate | std::ifstream::binary);
		if (!in.good()) return 0;

		long long sizeBytes = in.tellg();
		in.close();

		return sizeBytes;
	}

	double getSizeRatio(long long fileSizeBytes) {
		return _sizeLimitBytes / static_cast<double>(fileSizeBytes);
	}

	size_t getTruncStartIndex(const vector<string>& lines, long linesTotalLength, double sizeRatio) {
		long maxLength = static_cast<long>(linesTotalLength * sizeRatio);
		long currLength = linesTotalLength;
		size_t index = 0;

		while (currLength > maxLength && index < lines.size()) {
			currLength -= lines[index++].length();
		}

		return index;
	}

	pair<vector<string>, long> readAllLines() {
		ifstream in(_logFilePath);
		vector<string> lines{};
		long length = 0;
		string line;

		while (getline(in, line)) {
			lines.push_back(line);
			length += line.length();
		}

		in.close();
		return pair<vector<string>, long>(lines, length);
	}

	void writeToFile(const vector<string>& lines, size_t startIndex = 0) {
		ofstream f(_logFilePath);
		for(size_t i = startIndex; i < lines.size(); i++) {
			f << lines[i] << endl;
		}
		f.close();
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
