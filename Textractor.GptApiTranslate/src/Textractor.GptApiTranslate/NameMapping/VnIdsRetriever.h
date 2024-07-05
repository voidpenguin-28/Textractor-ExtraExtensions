
#pragma once
#include "../_Libraries/inihandler.h"
#include "../_Libraries/strhelper.h"
#include "../Extension.h"
#include "ProcessNameRetriever.h"
#include <memory>
#include <vector>


class VnIdsRetriever {
public:
	~VnIdsRetriever() { }
	virtual vector<string> getVnIds(SentenceInfoWrapper& sentInfoWrapper) const = 0;
};


class IniConfigVnIdsRetriever : public VnIdsRetriever {
public:
	IniConfigVnIdsRetriever(const ProcessNameRetriever& procNameRetriever, 
		const string& iniFilePath, const wstring& iniSectionName) 
		: _procNameRetriever(procNameRetriever), _iniHandler(iniFilePath), _iniSectionName(iniSectionName) { }

	vector<string> getVnIds(SentenceInfoWrapper& sentInfoWrapper) const override {
		wstring vnIdDelim;
		wstring vnIdsStr = parseVnIdsFromIni(vnIdDelim);
		if (vnIdsStr.empty()) return vector<string>();
		if (!contains(vnIdsStr, _keyValDelim)) return splitVnIds(vnIdsStr, vnIdDelim);

		vector<wstring> vnIds = splitVnIdsW(vnIdsStr, vnIdDelim);
		wstring appName = _procNameRetriever.getProcessName(sentInfoWrapper.getProcessIdD());
		return filterVnIds(vnIds, appName);
	}
private:
	static constexpr wchar_t _keyValDelim = L'=';
	const ProcessNameRetriever& _procNameRetriever;
	const IniFileHandler _iniHandler;
	const wstring _iniSectionName;

	wstring parseVnIdsFromIni(wstring& vnIdDelimOut) const {
		static const wstring VN_IDS_KEY = L"VnIds";
		static const wstring VN_ID_DELIM_KEY = L"VnIdDelim";

		unique_ptr<IniContents> ini(_iniHandler.readIni());
		wstring vnIdsStr = StrHelper::trim<wchar_t>(ini->getValue(_iniSectionName, VN_IDS_KEY), L"\"");
		vnIdDelimOut = ini->getValue(_iniSectionName, VN_ID_DELIM_KEY);

		return vnIdsStr;
	}

	bool contains(const wstring& str, const wchar_t ch) const {
		return str.find(ch) != wstring::npos;
	}

	bool startsWith(const wstring& str, const wstring& subStr) const {
		return str.find(subStr) == 0;
	}

	vector<string> splitVnIds(const wstring& vnIdsStr, const wstring& delim) const {
		return StrHelper::split<char>(StrHelper::convertFromW(vnIdsStr), StrHelper::convertFromW(delim), true);
	}

	vector<wstring> splitVnIdsW(const wstring& vnIdsStr, const wstring& delim) const {
		return StrHelper::split<wchar_t>(vnIdsStr, delim, true);
	}

	vector<string> filterVnIds(const vector<wstring>& vnIds, wstring appName) const {
		vector<string> nonMappedVnIds{};
		appName += _keyValDelim;


		for (const wstring& vnId : vnIds) {
			if (!contains(vnId, _keyValDelim)) {
				nonMappedVnIds.push_back(StrHelper::convertFromW(vnId));
			}
			else if (startsWith(vnId, appName)) {
				pair<wstring, wstring> appVnIdPair = splitToPair(vnId, _keyValDelim);
				return vector<string> { StrHelper::convertFromW(appVnIdPair.second) };
			}
		}

		return nonMappedVnIds;
	}

	pair<wstring, wstring> splitToPair(const wstring& str, const wchar_t delim) const {
		vector<wstring> splits = StrHelper::split<wchar_t>(str, delim, true);

		return splits.size() >= 2 ?
			pair<wstring, wstring>(splits[0], splits[1]) :
			pair<wstring, wstring>(str, L"");
	}
};
