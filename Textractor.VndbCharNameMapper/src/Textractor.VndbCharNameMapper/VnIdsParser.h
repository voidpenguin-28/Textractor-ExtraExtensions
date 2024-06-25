
#pragma once
#include "Libraries/strhelper.h"
#include "Extension.h"
#include "ExtensionConfig.h"
#include "ProcessNameRetriever.h"
#include <vector>


class VnIdsParser {
public:
	~VnIdsParser() { }
	virtual vector<string> parse(const ExtensionConfig& config, SentenceInfoWrapper& sentInfoWrapper) const = 0;
};


class DefaultVnIdsParser : public VnIdsParser {
public:
	DefaultVnIdsParser(const ProcessNameRetriever& procNameRetriever) : _procNameRetriever(procNameRetriever) { }

	vector<string> parse(const ExtensionConfig& config, SentenceInfoWrapper& sentInfoWrapper) const override {
		if (config.vnIds.empty()) return vector<string>();
		if (!contains(config.vnIds, _keyValDelim)) return splitVnIds(config);

		vector<wstring> vnIds = StrHelper::split<wchar_t>(config.vnIds, config.vnIdDelim, true);
		wstring appName = _procNameRetriever.getProcessName(sentInfoWrapper.getProcessIdD());
		return filterVnIds(vnIds, appName);
	}
private:
	static constexpr wchar_t _keyValDelim = L'=';
	const ProcessNameRetriever& _procNameRetriever;

	bool contains(const wstring& str, const wchar_t ch) const {
		return str.find(ch) != wstring::npos;
	}

	bool startsWith(const wstring& str, const wstring& subStr) const {
		return str.find(subStr) == 0;
	}

	vector<string> splitVnIds(const ExtensionConfig& config) const {
		return StrHelper::split<char>(StrHelper::convertFromW(config.vnIds), 
			StrHelper::convertFromW(config.vnIdDelim), true);
	}

	vector<wstring> splitVnIdsW(const ExtensionConfig& config) const {
		return StrHelper::split<wchar_t>(config.vnIds, config.vnIdDelim, true);
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
