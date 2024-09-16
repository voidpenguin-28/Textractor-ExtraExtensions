
#include "../Textractor.TranslationCache.Base/ExtensionDepsContainer.h"
#include "../Textractor.TranslationCache.Base/_Libraries/winmsg.h"

const string _backupModuleName = "TranslationCache.Read";
unique_ptr<ExtensionDepsContainer> _deps = nullptr;


inline void allocateResources(const HMODULE& hModule) {
	_deps = make_unique<DefaultExtensionDepsContainer>(hModule, true);
	_deps->getConfigRetriever().getConfig(true); // define default config if no config found
}

inline void clearCacheIfUnload() {
	try {
		ExtensionConfig config = _deps->getConfigRetriever().getConfig(false);
		if (config.clearCacheOnUnload) _deps->getCacheManager().clearCache();
	}
	catch (const exception&) {}
}

inline void deallocateResources() {
	if (_deps != nullptr) {
		clearCacheIfUnload();
	}

	_deps = nullptr;
}


BOOL WINAPI DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		allocateResources(hModule);
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
		if (_deps->isDisabled()) return false;
		ExtensionConfig config = _deps->getConfigRetriever().getConfig(false);
		SentenceInfoWrapper sentInfoWrapper(sentenceInfo);

		_deps->getConfigAdjustEvents().applyConfigAdjustments(config);
		sentence = _deps->getCacheManager().readCacheAndStoreTemp(sentence, sentInfoWrapper, config);
		return true;
	}
	catch (const exception& ex) {
		string appName = _deps != nullptr ? _deps->moduleName() : _backupModuleName;
		showErrorMessage(ex.what(), appName);
		return false;
	}
}