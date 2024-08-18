#pragma once
#include "TextLogger.h"
#include "File/Writer/DirCreatorFileWriter.h"
#include "File/Writer/WinApiFileWriter.h"
#include <memory>


class ExtensionDepsContainer {
public:
	virtual ~ExtensionDepsContainer() { }

	virtual string moduleName() = 0;
	virtual TextLogger& getTextLogger() = 0;
	virtual ConfigRetriever& getConfigRetriever() = 0;
};


class DefaultExtensionDepsContainer : public ExtensionDepsContainer {
public:
	DefaultExtensionDepsContainer(const HMODULE& handle) {
		_moduleName = getModuleName(handle);

		_fileTracker = make_unique<WinApiFileTracker>();
		_baseConfigRetriever = make_unique<IniConfigRetriever>(
			_iniFileName, StrHelper::convertToW(_moduleName));
		_mainConfigRetriever = make_unique<FileWatchMemCacheConfigRetriever>(
			*_baseConfigRetriever, *_fileTracker, _iniFileName);

		_keyGenerator = make_unique<DefaultThreadKeyGenerator>();
		_threadTracker = make_unique<MapThreadTracker>();
		_threadFilter = make_unique<DefaultThreadFilter>(*_keyGenerator, *_threadTracker);
		_procNameRetriever = make_unique<WinApiProcessNameRetriever>();
		_textHandler = make_unique<DefaultLoggerTextHandler>(*_procNameRetriever);
		_execRequirements = make_unique<DefaultExtExecRequirements>(*_threadFilter);
		_dirCreator = make_unique<WinApiDirectoryCreator>();

		_baseFileWriter = make_unique<PersistentWinApiFileWriter>();
		_mainFileWriter = make_unique<DirCreatorFileWriter>(*_baseFileWriter, *_dirCreator);

		_textLogger = make_unique<FileTextLogger>(*_mainConfigRetriever, *_keyGenerator,
			*_textHandler, *_threadTracker, *_mainFileWriter, *_execRequirements);
	}

	virtual string moduleName() override {
		return _moduleName;
	}

	virtual TextLogger& getTextLogger() override {
		return *_textLogger;
	}

	virtual ConfigRetriever& getConfigRetriever() override {
		return *_mainConfigRetriever;
	}
private:
	const string _iniFileName = "Textractor.ini";
	string _moduleName;

	unique_ptr<ConfigRetriever> _baseConfigRetriever = nullptr;
	unique_ptr<ConfigRetriever> _mainConfigRetriever = nullptr;
	unique_ptr<FileTracker> _fileTracker = nullptr;
	unique_ptr<DirectoryCreator> _dirCreator = nullptr;
	unique_ptr<ThreadKeyGenerator> _keyGenerator = nullptr;
	unique_ptr<ThreadTracker> _threadTracker = nullptr;
	unique_ptr<ThreadFilter> _threadFilter = nullptr;
	unique_ptr<ProcessNameRetriever> _procNameRetriever = nullptr;
	unique_ptr<LoggerTextHandler> _textHandler = nullptr;
	unique_ptr<ExtExecRequirements> _execRequirements = nullptr;
	unique_ptr<FileWriter> _baseFileWriter = nullptr;
	unique_ptr<FileWriter> _mainFileWriter = nullptr;
	unique_ptr<TextLogger> _textLogger = nullptr;
};
