
#pragma once
#include "_Libraries/strhelper.h"
#include <string>
#include <vector>
using namespace std;

class TextFormatter {
public:
	virtual ~TextFormatter() { }
	virtual wstring format(wstring text) const = 0;
};


class DefaultTextFormatter : public TextFormatter {
public:
	wstring format(wstring text) const override {
		return trimWS(text);
	}
private:
	const vector<wstring> _wsStrs = { L" ", L"　", L"\r", L"\n", L"\t" };

	wstring trimWS(wstring text) const {
		for (const wstring& ws : _wsStrs) {
			text = StrHelper::trim<wchar_t>(text, ws);
		}

		return text;
	}
};
