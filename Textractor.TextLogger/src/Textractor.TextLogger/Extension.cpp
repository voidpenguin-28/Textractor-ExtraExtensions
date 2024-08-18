
#include "_Libraries/winmsg.h"
#include "Extension.h"
#include "ExtensionDepsContainer.h"

const string _backupModuleName = "TextLogger";
ExtensionDepsContainer* _deps = nullptr;

inline void allocateResources(const HMODULE& handle) {
	_deps = new DefaultExtensionDepsContainer(handle);
	_deps->getConfigRetriever().getConfig(true); // initialize default config if not found in ini
}

inline void deallocateResources() {
	if (_deps != nullptr) delete _deps;
	_deps = nullptr;
}


BOOL WINAPI DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		try {
			allocateResources(hModule);
		}
		catch (exception& ex) {
			showErrorMessage(ex.what(), _backupModuleName);
			deallocateResources();
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
		TextLogger& textLogger = _deps->getTextLogger();

		textLogger.log(sentence, sentInfoWrapper);
	}
	catch (exception& ex) {
		string appName = _deps != nullptr ? _deps->moduleName() : _backupModuleName;
		showErrorMessage(ex.what(), appName);
	}

	return false;
}