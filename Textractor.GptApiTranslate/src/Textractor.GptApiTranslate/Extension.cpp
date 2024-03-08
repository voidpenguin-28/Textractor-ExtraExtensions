
#include "Extension.h"
#include "ExtensionDepsContainer.h"
#include <string>

void applyTranslationToSentence(wstring& sentence, const wstring& translation);

ExtensionDepsContainer* _deps = nullptr;

inline void allocateResources() {
	_deps = new DefaultExtensionDepsContainer();
}

inline void deallocateResources() {
	if(_deps != nullptr) delete _deps;
	_deps = nullptr;
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		allocateResources();
		_deps->getConfigRetriever().getConfig(true); // initialize ini config for extension if not found
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
	wstring translation = _deps->getTranslator().translateW(sentInfoWrapper, sentence);
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
