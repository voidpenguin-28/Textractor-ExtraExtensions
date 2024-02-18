
#include "TranslationFormat.h"
#include "../_Libraries/strhelper.h"
#include <cctype>

#pragma warning(suppress: 4566)
const vector<pair<wstring, wstring>> DefaultTranslationFormatter::JP_QUOTE_MARKS = { 
	pair<wstring, wstring>(L"「", L"」"),
	pair<wstring, wstring>(L"『", L"』")
};


// *** PUBLIC

wstring DefaultTranslationFormatter::formatJp(wstring jpSent) const {
	for (const pair<wstring, wstring>& qtMrks : JP_QUOTE_MARKS) {
		if (contains(jpSent, qtMrks.first) && !contains(jpSent, qtMrks.second))
			jpSent += qtMrks.second;
		else if (contains(jpSent, qtMrks.second) && !contains(jpSent, qtMrks.first))
			jpSent = qtMrks.first + jpSent;
	}

	return jpSent;
}

wstring DefaultTranslationFormatter::formatTranslation(wstring translation) const {
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

bool DefaultTranslationFormatter::contains(const wstring& str, const wstring substr) const {
	return str.find(substr) != wstring::npos;
}

wstring DefaultTranslationFormatter::jpToEnQuoteMarks(wstring text) const {
	static wstring quoteChStr = wstring(1, QUOT_CH);

	for (const pair<wstring, wstring>& qtMrks : JP_QUOTE_MARKS) {
		text = StrHelper::replace(text, qtMrks.first, quoteChStr);
		text = StrHelper::replace(text, qtMrks.second, quoteChStr);
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
