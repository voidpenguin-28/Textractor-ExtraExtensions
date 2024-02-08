
#include "Extension.h"
#include "Translator.h"
#include <string>

void applyTranslationToSentence(wstring& sentence, const wstring& translation);

const string _iniFileName = "Textractor.ini";
const wstring _iniSectionName = L"GptApi-Translate";
const string _logFileName = "gpt-request-log.txt";

ConfigRetriever* _configRetriever = nullptr;
ThreadKeyGenerator* _threadKeyGenerator = nullptr;
ThreadTracker* _threadTracker = nullptr;
ThreadFilter* _threadFilter = nullptr;
MultiThreadMsgHistoryTracker* _msgHistTracker = nullptr;
GptMsgHandler* _gptMsgHandler = nullptr;
TranslationFormatter* _formatter = nullptr;
HttpClient* _httpClient = nullptr;
Logger* _logger = nullptr;
GptApiCaller* _gptApiCaller = nullptr;
Translator* _gptTranslator = nullptr;


inline void allocateResources() {
	_configRetriever = new IniConfigRetriever(_iniFileName, _iniSectionName);

	_threadKeyGenerator = new DefaultThreadKeyGenerator();
	_threadTracker = new MapThreadTracker();
	_threadFilter = new DefaultThreadFilter(*_threadKeyGenerator, *_threadTracker);
	_msgHistTracker = new DefaultMultiThreadMsgHistoryTracker(
		*_threadKeyGenerator, *_threadTracker, []() { return new MapMsgHistoryTracker(); });

	_gptMsgHandler = new DefaultGptMsgHandler();
	_formatter = new DefaultTranslationFormatter();
	_httpClient = new CurlProcHttpClient([]() { return _configRetriever->getConfig().customCurlPath; });
	_logger = new FileLogger(_logFileName);
	_gptApiCaller = new DefaultGptApiCaller(*_httpClient, *_logger,
		[]() { return DefaultGptApiCaller::GptConfig(_configRetriever->getConfig()); });

	_gptTranslator = new GptApiTranslator(*_configRetriever, *_threadFilter,
		*_msgHistTracker, *_gptApiCaller, *_gptMsgHandler, *_formatter);
}

inline void deallocateResources() {
	delete _gptTranslator;
	delete _gptApiCaller;
	delete _logger;
	delete _httpClient;
	delete _formatter;
	delete _gptMsgHandler;
	delete _msgHistTracker;
	delete _threadFilter;
	delete _threadTracker;
	delete _threadKeyGenerator;
	delete _configRetriever;
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		allocateResources();
		_configRetriever->getConfig(true); // initialize ini config for extension if not found
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
bool ProcessSentence(std::wstring& sentence, SentenceInfo sentenceInfo)
{
	//if (!sentenceInfo["current select"]) return false;

	SentenceInfoWrapper sentInfoWrapper(sentenceInfo);
	wstring translation = _gptTranslator->translateW(sentInfoWrapper, sentence);
	if (translation.empty()) return false;

	applyTranslationToSentence(sentence, translation);
	return true;
}

void applyTranslationToSentence(wstring& sentence, const wstring& translation) {
	size_t zeroWdSpaceIndex = sentence.rfind(ZERO_WIDTH_SPACE);

	// if a translation already exists in the string, then replace it with the gpt translation
	if (zeroWdSpaceIndex != wstring::npos)
		sentence = sentence.substr(0, zeroWdSpaceIndex);

	(sentence += ZERO_WIDTH_SPACE) += L" \n";
	sentence += translation;
}
