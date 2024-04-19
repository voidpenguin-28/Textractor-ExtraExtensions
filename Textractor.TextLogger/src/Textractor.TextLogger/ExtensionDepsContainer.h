#pragma once
#include "TextLogger.h"
#include <memory>

class ExtensionDepsContainer {
public:
	virtual ~ExtensionDepsContainer() { }

	virtual TextLogger& getTextLogger() = 0;
	virtual ConfigRetriever& getConfigRetriever() = 0;
};

class DefaultExtensionDepsContainer : public ExtensionDepsContainer {
public:
	DefaultExtensionDepsContainer(const string& moduleName) {
		_configRetriever = make_unique<IniConfigRetriever>(_iniFileName, convertToW(moduleName));
		_dirCreator = make_unique<WinApiDirectoryCreator>();
		_keyGenerator = make_unique<DefaultThreadKeyGenerator>();
		_threadTracker = make_unique<MapThreadTracker>();
		_threadFilter = make_unique<DefaultThreadFilter>();
		_procNameRetriever = make_unique<WinApiProcessNameRetriever>();
		_textHandler = make_unique<DefaultLoggerTextHandler>(*_procNameRetriever);
		_writerFactory = make_unique<OFStreamFileWriterFactory>(*_dirCreator);
		_writerManager = make_unique<FactoryFileWriterManager>(*_writerFactory);

		_textLogger = make_unique<FileTextLogger>(*_configRetriever, *_keyGenerator,
			*_threadFilter, *_textHandler, *_threadTracker, *_writerManager);
	}

	virtual TextLogger& getTextLogger() {
		return *_textLogger;
	}

	virtual ConfigRetriever& getConfigRetriever() {
		return *_configRetriever;
	}
private:
	const string _iniFileName = "Textractor.ini";

	unique_ptr<ConfigRetriever> _configRetriever = nullptr;
	unique_ptr<DirectoryCreator> _dirCreator = nullptr;
	unique_ptr<ThreadKeyGenerator> _keyGenerator = nullptr;
	unique_ptr<ThreadTracker> _threadTracker = nullptr;
	unique_ptr<ThreadFilter> _threadFilter = nullptr;
	unique_ptr<ProcessNameRetriever> _procNameRetriever = nullptr;
	unique_ptr<LoggerTextHandler> _textHandler = nullptr;
	unique_ptr<FileWriterFactory> _writerFactory = nullptr;
	unique_ptr<FileWriterManager> _writerManager = nullptr;
	unique_ptr<TextLogger> _textLogger = nullptr;
};
