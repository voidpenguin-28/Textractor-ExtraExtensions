
#include "TranslationFormat.h"
#include "../_Libraries/strhelper.h"
#include <cctype>

#pragma warning(suppress: 4566)
const vector<wstring> DefaultTranslationFormatter::JP_QUOTE_MARKS = { L"「", L"」", L"『", L"』" };


// *** PUBLIC

wstring DefaultTranslationFormatter::format(wstring translation) const {
	if (translation.empty()) return translation;

	// ensure jp quote marks are placed with english equivalent "
	translation = jpToEnQuoteMarks(translation);

	size_t quotIndex = translation.find(QUOT_CH);

	if (between(quotIndex, 0) && translation[translation.length() - 1] == QUOT_CH) {
		// ensure a space exists before dialogue line starts
		if (translation[quotIndex - 1] != SPACE_CH)
			translation = translation.insert(quotIndex++, 1, SPACE_CH);

		// ensure that speaker name before dialogue line is proceeded by a ':'
		size_t spaceIndex = quotIndex - 1;
		if (between(spaceIndex, 2, 12, false) && !isSpaceOrPunct(translation[spaceIndex - 1]))
			translation = translation.insert(spaceIndex, 1, COLON_CH);
	}

	return translation;
}


// *** PRIVATE

wstring DefaultTranslationFormatter::jpToEnQuoteMarks(wstring text) const {
	static wstring quoteChStr = wstring(1, QUOT_CH);

	for (const wstring& qtMrk : JP_QUOTE_MARKS) {
		text = StrHelper::replace(text, qtMrk, quoteChStr);
	}

	return text;
}

bool DefaultTranslationFormatter::between(size_t i, size_t atLeast, size_t atMost, bool exclusive) const {
	return exclusive ?
		i > atLeast && i < atMost :
		i >= atLeast && i <= atMost;
}

bool DefaultTranslationFormatter::isSpaceOrPunct(wchar_t c) const {
	return isspace(c) || ispunct(c);
}
