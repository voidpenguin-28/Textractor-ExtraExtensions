
#pragma once
#include "_Libraries/strhelper.h"
#include "TextFormatter.h"

class TextMapper {
public:
	virtual ~TextMapper() { }
	virtual wstring merge(const wstring& text1, const wstring& text2) const = 0;
	virtual pair<wstring, wstring> split(wstring text) const = 0;
};


class TextractorTextMapper : public TextMapper {
public:
	TextractorTextMapper(const TextFormatter& formatter) : _formatter(formatter) { }

	wstring merge(const wstring& text1, const wstring& text2) const override {
		return _formatter.format(text1) + ZERO_WIDTH_SPACE + NEW_LINE + _formatter.format(text2);
	}

	pair<wstring, wstring> split(wstring text) const override {
		vector<wstring> splitStrs = StrHelper::split<wchar_t>(text, ZERO_WIDTH_SPACE);
		if (splitStrs.size() < 2) return pair<wstring, wstring>(text, L"");

		wstring str1 = _formatter.format(splitStrs[0]);
		wstring str2 = _formatter.format(splitStrs.back());
		return pair<wstring, wstring>(str1, str2);
	}
private:
	static constexpr wchar_t ZERO_WIDTH_SPACE = L'\x200b';
	static constexpr wchar_t NEW_LINE = L'\n';
	const TextFormatter& _formatter;
};


class DelimTextMapper : public TextMapper {
public:
	DelimTextMapper(const TextFormatter& formatter, const wstring& delim) 
		: _formatter(formatter), _delim(delim) { }
	DelimTextMapper(const TextFormatter& formatter) : DelimTextMapper(formatter, _defaultDelim) { }

	wstring merge(const wstring& text1, const wstring& text2) const override {
		return _formatter.format(text1) + _delim + _formatter.format(text2);
	}

	pair<wstring, wstring> split(wstring text) const override {
		pair<wstring, wstring> textPair = splitToPair(text, _delim);
		return textPair;
	}
private:
	const wstring _defaultDelim = L"|~|";
	const TextFormatter& _formatter;
	const wstring _delim;


	pair<wstring, wstring> splitToPair(const wstring& text, const wstring& delim) const {
		size_t delimIndex = text.find(delim);
		if (delimIndex == wstring::npos) return pair<wstring, wstring>(text, L"");
		
		wstring str1 = _formatter.format(text.substr(0, delimIndex));
		wstring str2 = _formatter.format(text.substr(delimIndex + delim.length()));
		return pair<wstring, wstring>(str1, str2);
	}
};
