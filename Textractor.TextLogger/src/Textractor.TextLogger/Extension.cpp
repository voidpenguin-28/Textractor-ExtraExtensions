
#include "Extension.h"
#include "TextLogger.h"
#include <cstdint>
#include <string> 
#include <windows.h>
using namespace std;

const string _iniFileName = "Textractor.ini";
const string _defaultName = "TextLogger";

ConfigRetriever* _configRetriever = nullptr;
DirectoryCreator* _dirCreator = nullptr;
ThreadKeyGenerator* _keyGenerator = nullptr;
ThreadTracker* _threadTracker = nullptr;
ThreadFilter* _threadFilter = nullptr;
ProcessNameRetriever* _procNameRetriever = nullptr;
LoggerTextHandler* _textHandler = nullptr;
FileWriterFactory* _writerFactory = nullptr;
FileWriterManager* _writerManager = nullptr;
TextLogger* _textLogger = nullptr;


inline void showErrorMessage(const char* message) {
	MessageBoxA(nullptr, message, "TextLogger-Error", MB_ICONERROR | MB_OK);
}

inline string getModuleName(const HMODULE& handle, const string& defaultName = _defaultName) {
	try {
		wchar_t buffer[1024];
		GetModuleFileName(handle, buffer, sizeof(buffer) / sizeof(wchar_t));

		string module = convertFromW(buffer);
		size_t pathDelimIndex = module.rfind('\\');
		if (pathDelimIndex != string::npos) module = module.substr(pathDelimIndex + 1);

		size_t extIndex = module.rfind('.');
		if (extIndex != string::npos) module = module.erase(extIndex);

		return module;
	}
	catch (exception& ex) {
		string errMsg = "Failed to retrieve extension name. Defaulting to name '" + defaultName + "'\n" + ex.what();
		showErrorMessage(errMsg.c_str());
		return defaultName;
	}
}

inline void allocateResources(const HMODULE& handle) {
	string moduleName = getModuleName(handle);
	_configRetriever = new IniConfigRetriever(_iniFileName, convertToW(moduleName));
	_dirCreator = new WinApiDirectoryCreator();
	_keyGenerator = new DefaultThreadKeyGenerator();
	_threadTracker = new MapThreadTracker();
	_threadFilter = new DefaultThreadFilter();
	_procNameRetriever = new WinApiProcessNameRetriever();
	_textHandler = new DefaultLoggerTextHandler(*_procNameRetriever);
	_writerFactory = new OFStreamFileWriterFactory(*_dirCreator);
	_writerManager = new FactoryFileWriterManager(*_writerFactory);

	_textLogger = new FileTextLogger(*_configRetriever, *_keyGenerator, 
		*_threadFilter, *_textHandler, *_threadTracker, *_writerManager);
}

inline void deallocateResources() {
	delete _textLogger;
	delete _writerManager;
	delete _writerFactory;
	delete _textHandler;
	delete _procNameRetriever;
	delete _threadTracker;
	delete _threadFilter;
	delete _keyGenerator;
	delete _dirCreator;
	delete _configRetriever;
}


BOOL WINAPI DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		try {
			allocateResources(hModule);
			_configRetriever->getConfig(true); // initialize default config if not found in ini
		}
		catch (exception& ex) {
			showErrorMessage(ex.what());
			throw;
		}
		break;
	case DLL_PROCESS_DETACH:
		deallocateResources();
		break;
	}

	return TRUE;
}

/*
	Param sentence: sentence received by Textractor (UTF-16). Can be modified, Textractor will receive this modification only if true is returned.
	Param sentenceInfo: contains miscellaneous info about the sentence (see README).
	Return value: whether the sentence was modified.
	Textractor will display the sentence after all extensions have had a chance to process and/or modify it.
	The sentence will be destroyed if it is empty or if you call Skip().
	This function may be run concurrently with itself: please make sure it's thread safe.
	It will not be run concurrently with DllMain.
*/
bool ProcessSentence(wstring& sentence, SentenceInfo sentenceInfo)
{
	try {
		SentenceInfoWrapper sentInfoWrapper(sentenceInfo);
		_textLogger->log(sentence, sentInfoWrapper);
	}
	catch (exception& ex) {
		showErrorMessage(ex.what());
	}

	return false;
}