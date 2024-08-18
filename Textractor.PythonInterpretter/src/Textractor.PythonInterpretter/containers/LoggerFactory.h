
#pragma once
#include "../logging/Loggers.h"
#include "../File/Writer/WinApiFileWriter.h"
#include <memory>


class LoggerFactory {
public:
	virtual ~LoggerFactory() { }
	virtual Logger* createLogger(const string& logFilePath,
		Logger::Events& loggerEvents, Logger::Level logLevel) = 0;
};

class NoLoggerFactory : public LoggerFactory {
public:
	Logger* createLogger(const string& logFilePath, Logger::Events& loggerEvents, Logger::Level logLevel) override
	{
		return new NoLogger();
	}
};

class PersistentWinApiFileWriterLoggerManager : public LoggerFactory {
public:
	PersistentWinApiFileWriterLoggerManager(const uint64_t logSizeLimitBytes = LOG_SIZE_LIMIT_BYTES_DEF)
		: _logSizeLimitBytes(logSizeLimitBytes)
	{
		_fileReader = make_unique<FstreamFileReader>();
		_fileWriter = make_unique<PersistentWinApiFileWriter>();
		_fileTruncater = make_unique<DefaultFileTruncater>(*_fileReader, *_fileWriter, _logSizeLimitBytes);
	}

	Logger* createLogger(const string& logFilePath, Logger::Events& loggerEvents, Logger::Level logLevel) override
	{
		return new FileWriterLogger(*_fileWriter, *_fileTruncater, logFilePath, loggerEvents, logLevel);
	}
private:
	static constexpr uint64_t LOG_SIZE_LIMIT_BYTES_DEF = 10 * 1024 * 1024;
	const uint64_t _logSizeLimitBytes;

	unique_ptr<FileReader> _fileReader = nullptr;
	unique_ptr<FileTruncater> _fileTruncater = nullptr;
	unique_ptr<FileWriter> _fileWriter = nullptr;
};
