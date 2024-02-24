
#include "Extension.h"
#include "NameMappingManager.h"
#include <string>
using namespace std;


const string _configIniFileName = "Textractor.ini";
const string _defaultName = "VNDB-CharNameMapper";
bool _namesLoaded = false;

ConfigRetriever* _configRetriever = nullptr;
HttpClient* _httpClient = nullptr;
HtmlParser* _htmlParser = nullptr;
NameRetriever* _httpNameRetriever = nullptr;
NameRetriever* _iniCacheNameRetriever = nullptr;
NameRetriever* _memCacheNameRetriever = nullptr;
NameMapper* _nameMapper = nullptr;
NameMappingManager* _mappingManager = nullptr;


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


inline void allocateResources(const HMODULE& hModule) {
	string moduleName = getModuleName(hModule);
	_configRetriever = new IniConfigRetriever(_configIniFileName, convertToW(moduleName));

	_httpClient = new CurlProcHttpClient([]() { return _configRetriever->getConfig().customCurlPath; });
	_htmlParser = new VndbHtmlParser();
	_nameMapper = new DefaultNameMapper();

	_httpNameRetriever = new VndbHttpNameRetriever(
		[]() { return _configRetriever->getConfig().urlTemplate; }, 
		*_httpClient, *_htmlParser
	);

	const function<bool()> reloadCacheGetter = []() {
		if (_namesLoaded) return false;
		ExtensionConfig config = _configRetriever->getConfig();
		_namesLoaded = true;
		return config.reloadCacheOnLaunch;
	};
	_iniCacheNameRetriever = new IniFileCacheNameRetriever(
		moduleName + ".ini", *_httpNameRetriever, reloadCacheGetter);
	_memCacheNameRetriever = new MemoryCacheNameRetriever(*_iniCacheNameRetriever, reloadCacheGetter);
	
	_mappingManager = new DefaultNameMappingManager(*_configRetriever, *_nameMapper, *_memCacheNameRetriever);
}

inline void deallocateResources() {
	delete _mappingManager;
	delete _nameMapper;
	delete _memCacheNameRetriever;
	delete _iniCacheNameRetriever;
	delete _httpNameRetriever;
	delete _htmlParser;
	delete _configRetriever;
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			try {
				allocateResources(hModule);
				_configRetriever->getConfig(true); // define default config if no config found
			}
			catch (exception& ex) {
				showErrorMessage(ex.what());
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
		sentence = _mappingManager->applyAllNameMappings(sentence, sentInfoWrapper);
		return true;
	}
	catch (exception& ex) {
		showErrorMessage(ex.what());
		return false;
	}
}