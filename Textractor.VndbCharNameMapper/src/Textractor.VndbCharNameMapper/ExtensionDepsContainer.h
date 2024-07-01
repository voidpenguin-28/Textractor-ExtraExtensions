
#pragma once
#include "HtmlParsers/StrFindVndbHtmlParser.h"
#include "NameMappingManager.h"
#include <memory>
#include <string>
using namespace std;

class ExtensionDepsContainer {
public:
	virtual ~ExtensionDepsContainer() { }

	virtual NameMappingManager& getNameMappingManager() = 0;
	virtual ConfigRetriever& getConfigRetriever() = 0;
};

class DefaultExtensionDepsContainer : public ExtensionDepsContainer {
public:
	DefaultExtensionDepsContainer(const string& moduleName) {
		_configRetriever = make_unique<IniConfigRetriever>(_configIniFileName, StrHelper::convertToW(moduleName));

		_httpClient = make_unique<CurlProcHttpClient>(
			[this]() { return _configRetriever->getConfig().customCurlPath; });

		_vndbGenderStrMapper = make_unique<VndbHtmlGenderStrMapper>();
		_outGenderStrMapper = make_unique<DefaultGenderStrMapper>();
		_charMappingConverter = make_unique<DefaultCharMappingConverter>();

		_htmlParser = make_unique<StrFindVndbHtmlParser>(*_charMappingConverter, *_vndbGenderStrMapper);
		_nameMapper = make_unique<DefaultNameMapper>(*_outGenderStrMapper);

		_httpNameRetriever = make_unique<VndbHttpNameRetriever>(
			[this]() { return _configRetriever->getConfig().urlTemplate; },
			*_httpClient, *_htmlParser
		);

		const function<bool()> reloadCacheGetter = [this]() {
			if (_namesLoaded) return false;
			ExtensionConfig config = _configRetriever->getConfig();
			_namesLoaded = true;
			return config.reloadCacheOnLaunch;
		};

		_iniCacheNameRetriever = make_unique<IniFileCacheNameRetriever>(
			moduleName + ".ini", *_httpNameRetriever, *_outGenderStrMapper, reloadCacheGetter);
		_memCacheNameRetriever = make_unique<MemoryCacheNameRetriever>(*_iniCacheNameRetriever, reloadCacheGetter);

		_procNameRetriever = make_unique<WinApiProcessNameRetriever>();
		_vnIdsParser = make_unique<DefaultVnIdsParser>(*_procNameRetriever);

		_mappingManager = make_unique<DefaultNameMappingManager>(
			*_configRetriever, *_nameMapper, *_memCacheNameRetriever, *_vnIdsParser);
	}

	virtual NameMappingManager& getNameMappingManager() {
		return *_mappingManager;
	}

	virtual ConfigRetriever& getConfigRetriever() {
		return *_configRetriever;
	}
private:
	const string _configIniFileName = "Textractor.ini";
	bool _namesLoaded = false;

	unique_ptr<ConfigRetriever> _configRetriever = nullptr;
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
	unique_ptr<NameMappingManager> _mappingManager = nullptr;
};
