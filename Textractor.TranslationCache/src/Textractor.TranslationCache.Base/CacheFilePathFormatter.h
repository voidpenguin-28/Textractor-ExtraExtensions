
#pragma once
#include "_Libraries/strhelper.h"


class CacheFilePathFormatter {
public:
	virtual ~CacheFilePathFormatter() { }
	virtual string format(wstring cacheFilePath) = 0;
	virtual string format(string cacheFilePath) = 0;
};


class DefaultCacheFilePathFormatter : public CacheFilePathFormatter {
public:
	DefaultCacheFilePathFormatter(const wstring& defaultFileName) : _defaultFileName(defaultFileName) { }

	string format(wstring cacheFilePath) override {
		if (cacheFilePath.find(L'/'))
			cacheFilePath = StrHelper::replace<wchar_t>(cacheFilePath, L"/", L"\\");

		if (cacheFilePath.empty() || cacheFilePath.find_last_of(_txtExtW) != cacheFilePath.length() - 1) {
			if (!cacheFilePath.empty() && cacheFilePath.back() != L'\\') cacheFilePath += L'\\';
			cacheFilePath += _defaultFileName + _txtExtW;
		}

		return StrHelper::convertFromW(cacheFilePath);
	}

	string format(string cacheFilePath) override {
		return format(StrHelper::convertToW(cacheFilePath));
	}
private:
	const wstring _txtExtW = L".txt";
	const string _txtExt = StrHelper::convertFromW(_txtExtW);
	const wstring _defaultFileName;
};
