
#pragma once
#include "Translator.h"
#include "_Libraries/regex/RE2Regex.h"
#include <memory>
#include <string>
using namespace std;


class ExtensionDepsContainer {
public:
	virtual ~ExtensionDepsContainer() { }
	virtual wstring getIdentifier() = 0;
	virtual ConfigRetriever& getConfigRetriever() = 0;
	virtual Logger& getLogger() = 0;
	virtual Translator& getTranslator() = 0;
};


class DefaultExtensionDepsContainer : public ExtensionDepsContainer {
public:
	DefaultExtensionDepsContainer() {
		_fileTracker = make_unique<WinApiFileTracker>();
		_baseConfigRetriever = make_unique<IniConfigRetriever>(_iniFileName, _iniSectionName);
		_mainConfigRetriever = make_unique<FileWatchMemCacheConfigRetriever>(
			*_baseConfigRetriever, *_fileTracker, _iniFileName);

		_threadKeyGenerator = make_unique<DefaultThreadKeyGenerator>();
		_threadTracker = make_unique<MapThreadTracker>();
		_threadFilter = make_unique<DefaultThreadFilter>(*_threadKeyGenerator, *_threadTracker);

		_msgHistTracker = make_unique<DefaultMultiThreadMsgHistoryTracker>(
			*_threadKeyGenerator, *_threadTracker, []() { return new MapMsgHistoryTracker(); });

		_genderStrMapper = make_unique<DefaultGenderStrMapper>();
		_procNameRetriever = make_unique<WinApiProcessNameRetriever>();
		_vnIdsRetriever = make_unique<IniConfigVnIdsRetriever>(
			*_procNameRetriever, _iniFileName, _vndbCharMapIniSectionName);
		_baseNameRetriever1 = make_unique<NoNameRetriever>();
		_baseNameRetriever2 = make_unique<IniFileCacheNameRetriever>(
			_vndbIniCacheFileName, *_baseNameRetriever1, *_genderStrMapper, []() { return false; });
		_mainNameRetriever = make_unique<MemoryCacheNameRetriever>(*_baseNameRetriever2, []() { return false; });

		_baseSysMsgCreator1 = make_unique<DefaultSysGptMsgCreator>();
		_baseSysMsgCreator2 = make_unique<NameMappingSysGptMsgCreator>(
			*_vnIdsRetriever, *_mainNameRetriever, *_genderStrMapper);
		_mainSysMsgCreator = make_unique<MultiGptMsgCreator>(
			vector<reference_wrapper<GptMsgCreator>>{ *_baseSysMsgCreator1, * _baseSysMsgCreator2 });

		_baseUserMsgCreator1 = make_unique<DefaultUserGptMsgCreator>();
		_baseUserMsgCreator2 = make_unique<MsgHistoryUserGptMsgCreator>(*_msgHistTracker);
		_mainUserMsgCreator = make_unique<MultiGptMsgCreator>(
			vector<reference_wrapper<GptMsgCreator>>{ *_baseUserMsgCreator1, *_baseUserMsgCreator2 });

		_gptLineParser = make_unique<DefaultGptLineParser>();
		_formatter = make_unique<DefaultTranslationFormatter>();
		_httpClient = make_unique<PerThreadHttpClient>([]() { return new LibCurlHttpClient(); });

		_logger = make_unique<FileLogger>(_logFileName);
		_execRequirements = make_unique<DefaultExtExecRequirements>(*_threadFilter);

		_apiMsgHelper = make_unique<DefaultApiMsgHelper>(*_mainConfigRetriever,
			[](const string& p) { return make_shared<RE2Regex>(p); });

		_gptApiCaller = make_unique<DefaultGptApiCaller>(*_httpClient, *_logger, *_apiMsgHelper);

		_gptTranslator = make_unique<GptApiTranslator>(*_mainConfigRetriever,
			*_execRequirements, *_threadFilter, *_msgHistTracker, *_gptApiCaller, 
			*_formatter, *_mainSysMsgCreator, *_mainUserMsgCreator, *_gptLineParser
		);
	}

	wstring getIdentifier() override {
		return _iniSectionName;
	}

	ConfigRetriever& getConfigRetriever() override {
		return *_mainConfigRetriever;
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
	const wstring _vndbCharMapIniSectionName = L"Textractor.VndbCharNameMapper";
	const string _vndbIniCacheFileName = StrHelper::convertFromW(_vndbCharMapIniSectionName) + ".ini";
	const string _logFileName = "gpt-request-log.txt";

	unique_ptr<FileTracker> _fileTracker = nullptr;
	unique_ptr<ConfigRetriever> _baseConfigRetriever = nullptr;
	unique_ptr<ConfigRetriever> _mainConfigRetriever = nullptr;

	unique_ptr<Logger> _logger = nullptr;
	unique_ptr<Translator> _gptTranslator = nullptr;
	unique_ptr<ExtExecRequirements> _execRequirements = nullptr;

	unique_ptr<ThreadKeyGenerator> _threadKeyGenerator = nullptr;
	unique_ptr<ThreadTracker> _threadTracker = nullptr;
	unique_ptr<ThreadFilter> _threadFilter = nullptr;
	unique_ptr<MultiThreadMsgHistoryTracker> _msgHistTracker = nullptr;
	unique_ptr<ApiMsgHelper> _apiMsgHelper = nullptr;

	unique_ptr<GenderStrMapper> _genderStrMapper = nullptr;
	unique_ptr<ProcessNameRetriever> _procNameRetriever = nullptr;
	unique_ptr<VnIdsRetriever> _vnIdsRetriever = nullptr;
	unique_ptr<NameRetriever> _baseNameRetriever1 = nullptr;
	unique_ptr<NameRetriever> _baseNameRetriever2 = nullptr;
	unique_ptr<NameRetriever> _mainNameRetriever = nullptr;

	unique_ptr<GptMsgCreator> _baseSysMsgCreator1 = nullptr;
	unique_ptr<GptMsgCreator> _baseSysMsgCreator2 = nullptr;
	unique_ptr<GptMsgCreator> _mainSysMsgCreator = nullptr;
	

	unique_ptr<GptMsgCreator> _baseUserMsgCreator1 = nullptr;
	unique_ptr<GptMsgCreator> _baseUserMsgCreator2 = nullptr;
	unique_ptr<GptMsgCreator> _mainUserMsgCreator = nullptr;
	
	unique_ptr<GptLineParser> _gptLineParser = nullptr;
	unique_ptr<TranslationFormatter> _formatter = nullptr;
	unique_ptr<HttpClient> _httpClient = nullptr;
	unique_ptr<GptApiCaller> _gptApiCaller = nullptr;
};
