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
	wstring formatJp(wstring jpSent) const;
	wstring formatTranslation(wstring translation) const;
private:
	static constexpr wchar_t QUOT_CH = L'\"';
	static constexpr wchar_t SPACE_CH = L' ';
	static constexpr wchar_t COLON_CH = L':';
	static const vector<pair<wstring, wstring>> JP_QUOTE_MARKS;
	
	bool contains(const wstring& str, const wstring substr) const;
	wstring jpToEnQuoteMarks(wstring text) const;
	bool between(size_t i, size_t atLeast, size_t atMost = wstring::npos, bool exclusive = true) const;
	bool isSpaceOrPunct(wchar_t c) const;
};
