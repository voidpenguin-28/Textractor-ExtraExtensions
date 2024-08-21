#pragma once

#include <string>
#include <vector>
using namespace std;

class TranslationFormatter {
public:
	virtual ~TranslationFormatter() { }
	virtual wstring formatJp(wstring jpSent) const = 0;
	virtual wstring formatTranslation(wstring translation) const = 0;
};

class DefaultTranslationFormatter : public TranslationFormatter {
public:
	wstring formatJp(wstring jpSent) const override {
		for (const pair<wstring, wstring>& qtMrks : JP_QUOTE_MARKS) {
			if (contains(jpSent, qtMrks.first) && !contains(jpSent, qtMrks.second))
				jpSent += qtMrks.second;
			else if (contains(jpSent, qtMrks.second) && !contains(jpSent, qtMrks.first))
				jpSent = qtMrks.first + jpSent;
		}

		return jpSent;
	}

	wstring formatTranslation(wstring translation) const override {
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
private:
	static constexpr wchar_t QUOT_CH = L'\"';
	static constexpr wchar_t SPACE_CH = L' ';
	static constexpr wchar_t COLON_CH = L':';
	#pragma warning(suppress: 4566)
	const vector<pair<wstring, wstring>> JP_QUOTE_MARKS = {
		pair<wstring, wstring>(L"「", L"」"),
		pair<wstring, wstring>(L"『", L"』")
	};
	
	bool contains(const wstring& str, const wstring substr) const {
		return str.find(substr) != wstring::npos;
	}

	wstring jpToEnQuoteMarks(wstring text) const {
		static wstring quoteChStr = wstring(1, QUOT_CH);

		for (const pair<wstring, wstring>& qtMrks : JP_QUOTE_MARKS) {
			text = StrHelper::replace(text, qtMrks.first, quoteChStr);
			text = StrHelper::replace(text, qtMrks.second, quoteChStr);
		}

		return text;
	}

	bool between(size_t i, size_t atLeast, size_t atMost = wstring::npos, bool exclusive = true) const {
		return exclusive ?
			i > atLeast && i < atMost :
			i >= atLeast && i <= atMost;
	}

	bool isSpaceOrPunct(wchar_t c) const {
		return isspace(c) || ispunct(c);
	}
};
