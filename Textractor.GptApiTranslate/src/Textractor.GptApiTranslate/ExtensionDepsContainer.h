
#pragma once
#include "Translator.h"
#include <memory>
#include <string>
using namespace std;


class ExtensionDepsContainer {
public:
	virtual  ConfigRetriever& getConfigRetriever() = 0;
	virtual Logger& getLogger() = 0;
	virtual Translator& getTranslator() = 0;
};


class DefaultExtensionDepsContainer : public ExtensionDepsContainer {
public:
	DefaultExtensionDepsContainer() {
		_configRetriever = unique_ptr<IniConfigRetriever>(new IniConfigRetriever(_iniFileName, _iniSectionName));

		_threadKeyGenerator = unique_ptr<DefaultThreadKeyGenerator>(new DefaultThreadKeyGenerator());
		_threadTracker = unique_ptr<MapThreadTracker>(new MapThreadTracker());
		_threadFilter = unique_ptr<DefaultThreadFilter>(
			new DefaultThreadFilter(*_threadKeyGenerator, *_threadTracker));

		_msgHistTracker = unique_ptr<DefaultMultiThreadMsgHistoryTracker>(new DefaultMultiThreadMsgHistoryTracker(
				*_threadKeyGenerator, *_threadTracker, []() { return new MapMsgHistoryTracker(); }));

		_gptMsgHandler = unique_ptr<DefaultGptMsgHandler>(new DefaultGptMsgHandler());
		_formatter = unique_ptr<DefaultTranslationFormatter>(new DefaultTranslationFormatter());
		_httpClient = unique_ptr<CurlProcHttpClient>(
			new CurlProcHttpClient([this]() { return _configRetriever->getConfig(false).customCurlPath; }));

		_logger = unique_ptr<FileLogger>(new FileLogger(_logFileName));
		_gptApiCaller = unique_ptr<DefaultGptApiCaller>(new DefaultGptApiCaller(*_httpClient, *_logger,
			[this]() { return DefaultGptApiCaller::GptConfig(_configRetriever->getConfig(false)); }));

		_gptTranslator = unique_ptr<GptApiTranslator>(new GptApiTranslator(*_configRetriever, 
			*_threadFilter, *_msgHistTracker, *_gptApiCaller, *_gptMsgHandler, *_formatter));
	}

	ConfigRetriever& getConfigRetriever() override {
		return *_configRetriever;
	}

	Logger& getLogger() override {
		return *_logger;
	}

	Translator& getTranslator() override {
		return *_gptTranslator;
	}
private:
	const string _iniFileName = "Textractor.ini";
	const wstring _iniSectionName = L"GptApi-Translate";
	const string _logFileName = "gpt-request-log.txt";

	unique_ptr<ConfigRetriever> _configRetriever = nullptr;
	unique_ptr<Logger> _logger = nullptr;
	unique_ptr<Translator> _gptTranslator = nullptr;

	unique_ptr<ThreadKeyGenerator> _threadKeyGenerator = nullptr;
	unique_ptr<ThreadTracker> _threadTracker = nullptr;
	unique_ptr<ThreadFilter> _threadFilter = nullptr;
	unique_ptr<MultiThreadMsgHistoryTracker> _msgHistTracker = nullptr;
	unique_ptr<GptMsgHandler> _gptMsgHandler = nullptr;
	unique_ptr<TranslationFormatter> _formatter = nullptr;
	unique_ptr<HttpClient> _httpClient = nullptr;
	unique_ptr<GptApiCaller> _gptApiCaller = nullptr;
};
