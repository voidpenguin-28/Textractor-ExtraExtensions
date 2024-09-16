
#pragma once
#include "../_Libraries/Locker.h"
#include "TextMapCache.h"


class MemoryTextMapCache : public TextMapCache {
public:
	MemoryTextMapCache(const TextFormatter& formatter) : _formatter(formatter) { }
	MemoryTextMapCache(const TextFormatter& formatter,
		const unordered_map<wstring, wstring>& preCache) : _formatter(formatter), _cache(preCache) { }

	bool keyExists(const wstring& key) const override {
		wstring formattedKey = _formatter.format(key);
		_writeLocker.waitForUnlock();
		return isInCache(formattedKey);
	}

	wstring readFromCache(const wstring& key) const override {
		wstring formattedKey = _formatter.format(key);
		_writeLocker.waitForUnlock();
		return isInCache(formattedKey) ? _cache.at(formattedKey) : L"";
	}

	unordered_map<wstring, wstring> readAllFromCache() const override {
		_writeLocker.waitForUnlock();
		return _cache;
	}

	void writeToCache(const wstring& key, const wstring& value) override {
		wstring formattedKey = _formatter.format(key);
		wstring formattedValue = _formatter.format(value);

		_writeLocker.lock([this, &formattedKey, formattedValue]() {
			writeToCacheBase(formattedKey, formattedValue);
		});
	}

	void writeAllToCache(const unordered_map<wstring, wstring> cache, bool reload = false) override {
		_writeLocker.lock([this, &cache, reload]() {
			if (reload) _cache.clear();

			for (const auto& textPair : cache) {
				formatAndWriteToCache(textPair.first, textPair.second);
			}
		});
	}

	void removeFromCache(const wstring& key) override {
		wstring formattedKey = _formatter.format(key);

		_writeLocker.lock([this, &formattedKey]() {
			removeFromCacheBase(formattedKey);
		});
	}

	void clearCache() override {
		_writeLocker.lock([this]() {
			_cache.clear();
		});
	}
private:
	const TextFormatter& _formatter;
	unordered_map<wstring, wstring> _cache;
	mutable BasicLocker _writeLocker;

	bool isInCache(const wstring& key) const {
		return _cache.find(key) != _cache.end();
	}

	void formatAndWriteToCache(const wstring& key, const wstring& value) {
		writeToCacheBase(_formatter.format(key), _formatter.format(value));
	}

	void writeToCacheBase(const wstring& key, const wstring& value) {
		_cache[key] = value;
	}

	void removeFromCacheBase(const wstring& key) {
		_cache.erase(key);
	}
};