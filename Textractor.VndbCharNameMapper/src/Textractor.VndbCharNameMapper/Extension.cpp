
#include "Libraries/winmsg.h";
#include "Extension.h"
#include "ExtensionDepsContainer.h"
#include <string>
using namespace std;

string _moduleName = "";
ExtensionDepsContainer* _deps = nullptr;


inline void allocateResources(const HMODULE& hModule) {
	_moduleName = getModuleName(hModule);
	_deps = new DefaultExtensionDepsContainer(_moduleName);
	_deps->getConfigRetriever().getConfig(true); // define default config if no config found
}

inline void deallocateResources() {
	if(_deps != nullptr) delete _deps;
	_deps = nullptr;
	_moduleName = "";
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
				showErrorMessage(ex.what(), _moduleName);
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
		NameMappingManager& mappingManager = _deps->getNameMappingManager();

		sentence = mappingManager.applyAllNameMappings(sentence, sentInfoWrapper);
		return true;
	}
	catch (exception& ex) {
		showErrorMessage(ex.what(), _moduleName);
		return false;
	}
}