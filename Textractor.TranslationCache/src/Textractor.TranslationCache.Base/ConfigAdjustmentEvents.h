
#pragma once
#include "_Libraries/Locker.h"
#include "Cache/TextMapCache.h"
#include "ExtensionConfig.h"


class ConfigAdjustmentEvents {
public:
	virtual ~ConfigAdjustmentEvents() { }
	virtual void applyConfigAdjustments(const ExtensionConfig& config) = 0;
};


class NoConfigAdjustmentEvents : public ConfigAdjustmentEvents {
	void applyConfigAdjustments(const ExtensionConfig& config) override { }
};


class DefaultReadConfigAdjustmentEvents : public ConfigAdjustmentEvents {
public:
	DefaultReadConfigAdjustmentEvents(TextMapCache& srcCache, TextMapCache& destCache)
		: _srcCache(srcCache), _destCache(destCache) { }

	void applyConfigAdjustments(const ExtensionConfig& config) override {
		if (config.cacheFilePath != _currCacheFilePath)
			changeCurrPathAndReloadCache(config.cacheFilePath);
	}
private:
	BasicLocker _locker;
	wstring _currCacheFilePath;
	TextMapCache& _srcCache;
	TextMapCache& _destCache;

	void changeCurrPathAndReloadCache(const wstring& newCacheFilePath) {
		_locker.lock([this, &newCacheFilePath]() {
			if (newCacheFilePath == _currCacheFilePath) return;
			_currCacheFilePath = newCacheFilePath;
			reloadMemCache();
		});
	}

	void reloadMemCache() {
		unordered_map<wstring, wstring> srcCacheMap = _srcCache.readAllFromCache();
		_destCache.writeAllToCache(srcCacheMap, true);
	}
};
