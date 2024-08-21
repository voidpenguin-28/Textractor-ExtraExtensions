
#pragma once
#include "HtmlParsers/StrFindVndbHtmlParser.h"
#include "NameMappingManager.h"
#include <memory>
#include <string>
using namespace std;

class ExtensionDepsContainer {
public:
	virtual ~ExtensionDepsContainer() { }

	virtual string moduleName() = 0;
	virtual NameMappingManager& getNameMappingManager() = 0;
	virtual ConfigRetriever& getConfigRetriever() = 0;
};

class DefaultExtensionDepsContainer : public ExtensionDepsContainer {
public:
	DefaultExtensionDepsContainer(const HMODULE& hModule) {
		_moduleName = getModuleName(hModule);
		_fileTracker = make_unique<WinApiFileTracker>();
		_iniConfigRetriever = make_unique<IniConfigRetriever>(_iniFileName, StrHelper::convertToW(_moduleName));
		_mainConfigRetriever = make_unique<FileWatchMemCacheConfigRetriever>(
			*_iniConfigRetriever, *_fileTracker, _iniFileName);

		_httpClient = make_unique<CurlProcHttpClient>(
			[this]() { return _mainConfigRetriever->getConfig(false).customCurlPath; });

		_vndbGenderStrMapper = make_unique<VndbHtmlGenderStrMapper>();
		_outGenderStrMapper = make_unique<DefaultGenderStrMapper>();
		_charMappingConverter = make_unique<DefaultCharMappingConverter>();

		_htmlParser = make_unique<StrFindVndbHtmlParser>(*_charMappingConverter, *_vndbGenderStrMapper);
		_nameMapper = make_unique<DefaultNameMapper>(*_outGenderStrMapper);

		_httpNameRetriever = make_unique<VndbHttpNameRetriever>(
			[this]() { return _mainConfigRetriever->getConfig(false).urlTemplate; },
			*_httpClient, *_htmlParser
		);

		const function<bool()> reloadCacheGetter = [this]() {
			if (_namesLoaded) return false;
			ExtensionConfig config = _mainConfigRetriever->getConfig(false);
			_namesLoaded = true;
			return config.reloadCacheOnLaunch;
		};

		_iniCacheNameRetriever = make_unique<IniFileCacheNameRetriever>(
			_moduleName + ".ini", *_httpNameRetriever, *_outGenderStrMapper, reloadCacheGetter);
		_memCacheNameRetriever = make_unique<MemoryCacheNameRetriever>(*_iniCacheNameRetriever, reloadCacheGetter);

		_procNameRetriever = make_unique<WinApiProcessNameRetriever>();
		_vnIdsParser = make_unique<DefaultVnIdsParser>(*_procNameRetriever);
		_execRequirements = make_unique<DefaultExtExecRequirements>();

		_mappingManager = make_unique<DefaultNameMappingManager>(*_mainConfigRetriever, 
			*_nameMapper, *_memCacheNameRetriever, *_vnIdsParser, *_execRequirements);
	}

	string moduleName() override {
		return _moduleName;
	}

	NameMappingManager& getNameMappingManager() override {
		return *_mappingManager;
	}

	ConfigRetriever& getConfigRetriever() override {
		return *_mainConfigRetriever;
	}
private:
	const string _iniFileName = "Textractor.ini";
	bool _namesLoaded = false;

	string _moduleName;
	unique_ptr<FileTracker> _fileTracker = nullptr;
	unique_ptr<ConfigRetriever> _iniConfigRetriever = nullptr;
	unique_ptr<ConfigRetriever> _mainConfigRetriever = nullptr;

	unique_ptr<HttpClient> _httpClient = nullptr;
	unique_ptr<GenderStrMapper> _vndbGenderStrMapper = nullptr;
	unique_ptr<GenderStrMapper> _outGenderStrMapper = nullptr;
	unique_ptr<CharMappingConverter> _charMappingConverter = nullptr;
	unique_ptr<HtmlParser> _htmlParser = nullptr;
	
	unique_ptr<NameRetriever> _httpNameRetriever = nullptr;
	unique_ptr<NameRetriever> _iniCacheNameRetriever = nullptr;
	unique_ptr<NameRetriever> _memCacheNameRetriever = nullptr;
	
	unique_ptr<NameMapper> _nameMapper = nullptr;
	unique_ptr<ProcessNameRetriever> _procNameRetriever = nullptr;
	unique_ptr<VnIdsParser> _vnIdsParser = nullptr;
	unique_ptr<ExtExecRequirements> _execRequirements = nullptr;
	unique_ptr<NameMappingManager> _mappingManager = nullptr;
};
