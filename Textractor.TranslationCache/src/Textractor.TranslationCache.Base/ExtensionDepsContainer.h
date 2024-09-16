
#pragma once
#include "../Textractor.TranslationCache.Base/_Libraries/strhelper.h"
#include "../Textractor.TranslationCache.Base/_Libraries/winmsg.h"
#include "../Textractor.TranslationCache.Base/Extension.h"
#include "../Textractor.TranslationCache.Base/CacheManager.h"
#include "../Textractor.TranslationCache.Base/Cache/FileTextMapCache.h"
#include "../Textractor.TranslationCache.Base/Cache/MemoryTextMapCache.h"
#include "../Textractor.TranslationCache.Base/Cache/SharedMemTextMapCache.h"
#include "../Textractor.TranslationCache.Base/File/FileTruncater.h"
#include "../Textractor.TranslationCache.Base/File/Writer/WinApiFileWriter.h"
#include "../Textractor.TranslationCache.Base/CacheFilePathFormatter.h"
#include "../Textractor.TranslationCache.Base/ConfigAdjustmentEvents.h"
#include <memory>


class ExtensionDepsContainer {
public:
	virtual ~ExtensionDepsContainer() { }
	virtual bool isDisabled() = 0;
	virtual string moduleName() = 0;
	virtual ConfigRetriever& getConfigRetriever() = 0;
	virtual ConfigAdjustmentEvents& getConfigAdjustEvents() = 0;
	virtual CacheManager& getCacheManager() = 0;
};


class DefaultExtensionDepsContainer : public ExtensionDepsContainer {
public:
	DefaultExtensionDepsContainer(const HMODULE hModule, bool readMode) {
		_moduleName = getModuleName(hModule);
		string moduleSuffix = readMode ? _readModuleSuffix : _writeModuleSuffix;
		_iniSection = getIniSectionName(_moduleName, moduleSuffix);
		if (_disabled) return;

		_fileTracker = make_unique<WinApiFileTracker>();
		_iniConfigRetriever = make_unique<IniConfigRetriever>(_iniFilePath, _iniSection);
		_mainConfigRetriever = make_unique<FileWatchMemCacheConfigRetriever>(
			*_iniConfigRetriever, *_fileTracker, _iniFilePath);
		ExtensionConfig config = getConfig(true);

		_formatter = make_unique<DefaultTextFormatter>();
		_textractorTextMapper = make_unique<TextractorTextMapper>(*_formatter);
		_cacheTextMapper = make_unique<DelimTextMapper>(*_formatter);
		_cacheFilePathFormatter = make_unique<DefaultCacheFilePathFormatter>(_iniSection);
		
		_fileDeleter = make_unique<CRemoveFileDeleter>();
		_fileReader = make_unique<FstreamFileReader>();
		_fileWriter = make_unique<PersistentWinApiFileWriter>();

		_fileTruncater = make_unique<DefaultFileTruncater>(*_fileReader, *_fileWriter,
			[this]() { return static_cast<long long>(getConfig().cacheFileLimitMb * 1024 * 1024); });
		truncateCacheFile(config);

		if (!readMode) {
			_configAdjustEvents = make_unique<NoConfigAdjustmentEvents>();
			_mainCache = unique_ptr<FileTextMapCache>(createFileTextMapCache());
		}
		else {
			auto& fileCache = _depsCacheMap["file"] = shared_ptr<FileTextMapCache>(createFileTextMapCache());
			unordered_map<wstring, wstring> fileCacheMap = fileCache->readAllFromCache();
			_mainCache = make_unique<MemoryTextMapCache>(*_formatter, fileCacheMap);
			
			_configAdjustEvents = make_unique<DefaultReadConfigAdjustmentEvents>(*fileCache, *_mainCache);
		}
		
		_sharedMemManager = make_unique<WinApiNamedSharedMemoryManager>();
		_tempStoreCache = make_unique<SharedMemTextMapCache>(*_formatter, *_sharedMemManager, false);
		_tempStore = make_unique<MapCacheTextTempStore>(
			*_tempStoreCache, *_cacheTextMapper, L"Textractor-TransCache-TEMP");
		
		_threadKeyGenerator = make_unique<DefaultThreadKeyGenerator>();
		_threadTracker = make_unique<MapThreadTracker>();
		_threadFilter = make_unique<DefaultThreadFilter>(*_threadKeyGenerator, *_threadTracker);
		
		_execRequirements = make_unique<DefaultExtExecRequirements>(*_threadFilter);
		_cacheManager = make_unique<DefaultCacheManager>( 
			*_mainCache, *_tempStore, *_textractorTextMapper, *_execRequirements, true);
	}

	~DefaultExtensionDepsContainer() {
		_depsCacheMap.clear();

		ExtensionConfig config = getConfig();
		truncateCacheFile(config);
	}

	bool isDisabled() override {
		return _disabled;
	}

	string moduleName() override {
		return _moduleName;
	}

	ConfigRetriever& getConfigRetriever() override {
		return *_mainConfigRetriever;
	}

	ConfigAdjustmentEvents& getConfigAdjustEvents() override {
		return *_configAdjustEvents;
	}

	CacheManager& getCacheManager() override {
		return *_cacheManager;
	}
private:
	bool _disabled = false;
	const string _iniFilePath = "Textractor.ini";
	const string _readModuleSuffix = ".Read";
	const string _writeModuleSuffix = ".Write";
	wstring _iniSection;
	string _moduleName;

	unique_ptr<FileTracker> _fileTracker = nullptr;
	unique_ptr<ConfigRetriever> _iniConfigRetriever = nullptr;
	unique_ptr<ConfigRetriever> _mainConfigRetriever = nullptr;
	unique_ptr<TextFormatter> _formatter = nullptr;
	unique_ptr<TextMapper> _textractorTextMapper = nullptr;
	unique_ptr<TextMapper> _cacheTextMapper = nullptr;

	unique_ptr<FileDeleter> _fileDeleter = nullptr;
	unique_ptr<FileReader> _fileReader = nullptr;
	unique_ptr<FileWriter> _fileWriter = nullptr;
	unique_ptr<FileTruncater> _fileTruncater = nullptr;

	unique_ptr<CacheFilePathFormatter> _cacheFilePathFormatter = nullptr;
	unordered_map<string, shared_ptr<TextMapCache>> _depsCacheMap{};
	unique_ptr<TextMapCache> _mainCache = nullptr;

	unique_ptr<SharedMemoryManager> _sharedMemManager = nullptr;
	unique_ptr<TextMapCache> _tempStoreCache = nullptr;
	unique_ptr<TextTempStore> _tempStore = nullptr;

	unique_ptr<ThreadKeyGenerator> _threadKeyGenerator = nullptr;
	unique_ptr<ThreadTracker> _threadTracker = nullptr;
	unique_ptr<ThreadFilter> _threadFilter = nullptr;

	unique_ptr<ConfigAdjustmentEvents> _configAdjustEvents = nullptr;
	unique_ptr<ExtExecRequirements> _execRequirements = nullptr;
	unique_ptr<CacheManager> _cacheManager = nullptr;

	wstring getIniSectionName(string moduleName, const string& moduleSuffix) {
		size_t suffixIndex = moduleName.rfind(moduleSuffix);

		if (suffixIndex != moduleName.length() - moduleSuffix.length()) {
			showErrorMessage("Extension name must end with '" + moduleSuffix + "' (before file extension) for extension to work correctly. Extension functionality disabled.\nEx: 'Textractor.TranslationCache." + moduleSuffix + "'\nCurrent Extension Name: " + moduleName, moduleName);
			_disabled = true;
			return L"";
		}

		moduleName.erase(suffixIndex);
		return StrHelper::convertToW(moduleName);
	}

	ExtensionConfig getConfig(bool saveDefault = false) {
		return _mainConfigRetriever->getConfig(saveDefault);
	}

	FileTextMapCache* createFileTextMapCache() {
		return new FileTextMapCache(*_fileWriter, *_fileReader, 
			*_fileDeleter, *_formatter, *_cacheTextMapper,
			[this]() { return _cacheFilePathFormatter->format(getConfig().cacheFilePath); }
		);
	}

	void truncateCacheFile(const ExtensionConfig config) {
		_fileTruncater->truncateFile(StrHelper::convertFromW(config.cacheFilePath));
	}
};
