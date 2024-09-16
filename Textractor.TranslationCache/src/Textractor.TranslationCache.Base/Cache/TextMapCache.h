
#pragma once
#include "../ExtensionConfig.h"
#include "../TextMapper.h"
#include <unordered_map>


class TextMapCacheReader {
public:
	virtual ~TextMapCacheReader() { }
	virtual bool keyExists(const wstring& key) const = 0;
	virtual wstring readFromCache(const wstring& key) const = 0;
	virtual unordered_map<wstring, wstring> readAllFromCache() const = 0;
};

class TextMapCacheWriter {
public:
	virtual ~TextMapCacheWriter() { }
	virtual void writeToCache(const wstring& key, const wstring& value) = 0;
	virtual void writeAllToCache(const unordered_map<wstring, wstring> cache, bool reload = false) = 0;
	virtual void removeFromCache(const wstring& key) = 0;
	virtual void clearCache() = 0;
};

class TextMapCache : public TextMapCacheReader, public TextMapCacheWriter {
public:
	virtual ~TextMapCache() { }
};


class NoTextMapCache : public TextMapCache {
public:
	bool keyExists(const wstring& key) const override { return false; }
	wstring readFromCache(const wstring& key) const override { return L""; }
	unordered_map<wstring, wstring> readAllFromCache() const override { return unordered_map<wstring, wstring>{}; }
	void writeToCache(const wstring& key, const wstring& value) override { }
	void writeAllToCache(const unordered_map<wstring, wstring> cache, bool reload = false) override { }
	void removeFromCache(const wstring& key) override { }
	void clearCache() override { }
};


class ModularTextMapCache : public TextMapCache {
public:
	ModularTextMapCache(TextMapCacheReader& cacheReader, TextMapCacheWriter& cacheWriter) 
		: _cacheReader(cacheReader), _cacheWriter(cacheWriter) { }

	bool keyExists(const wstring& key) const override {
		return _cacheReader.keyExists(key);
	}

	wstring readFromCache(const wstring& key) const override {
		return _cacheReader.readFromCache(key);
	}

	unordered_map<wstring, wstring> readAllFromCache() const override {
		return _cacheReader.readAllFromCache();
	}

	void writeToCache(const wstring& key, const wstring& value) override {
		_cacheWriter.writeToCache(key, value);
	}

	void writeAllToCache(const unordered_map<wstring, wstring> cache, bool reload = false) override {
		_cacheWriter.writeAllToCache(cache, reload);
	}

	void removeFromCache(const wstring& key) override {
		_cacheWriter.removeFromCache(key);
	}

	void clearCache() override {
		_cacheWriter.clearCache();
	}
private:
	TextMapCacheReader& _cacheReader;
	TextMapCacheWriter& _cacheWriter;
};


class ConditionalTextMapCache : public TextMapCache {
public:
	ConditionalTextMapCache(TextMapCache& conditionCache, 
		TextMapCache& fallbackCache, const function<bool()>& condition) 
		: _conditionCache(conditionCache), _fallbackCache(fallbackCache), _condition(condition) { }
	
	bool keyExists(const wstring& key) const override {
		TextMapCache& cacher = getCache();
		return cacher.keyExists(key);
	}

	wstring readFromCache(const wstring& key) const override {
		TextMapCache& cacher = getCache();
		return cacher.readFromCache(key);
	}

	unordered_map<wstring, wstring> readAllFromCache() const override {
		TextMapCache& cacher = getCache();
		return cacher.readAllFromCache();
	}

	void writeToCache(const wstring& key, const wstring& value) override {
		TextMapCache& cacher = getCache();
		cacher.writeToCache(key, value);
	}

	void writeAllToCache(const unordered_map<wstring, wstring> cache, bool reload = false) override {
		TextMapCache& cacher = getCache();
		cacher.writeAllToCache(cache, reload);
	}

	void removeFromCache(const wstring& key) override {
		TextMapCache& cacher = getCache();
		cacher.removeFromCache(key);
	}

	void clearCache() override {
		TextMapCache& cacher = getCache();
		cacher.clearCache();
	}
private:
	TextMapCache& _conditionCache;
	TextMapCache& _fallbackCache;
	const function<bool()> _condition;

	TextMapCache& getCache() const {
		return _condition() ? _conditionCache : _fallbackCache;
	}
};


class MultiTextMapCache : public TextMapCache {
public:
	MultiTextMapCache(const vector<reference_wrapper<TextMapCache>> caches) : _caches(caches) { }
	
	bool keyExists(const wstring& key) const override {
		for (auto& cacher : _caches) {
			if (cacher.get().keyExists(key)) return true;
		}

		return false;
	}

	wstring readFromCache(const wstring& key) const override {
		wstring value;

		for (auto& cacher : _caches) {
			value = cacher.get().readFromCache(key);
			if (!value.empty()) return value;
		}

		return L"";
	}

	unordered_map<wstring, wstring> readAllFromCache() const override {
		unordered_map<wstring, wstring> mapCache{}, tempCache;

		for (auto& cacher : _caches) {
			tempCache = cacher.get().readAllFromCache();
			mapCache.insert(tempCache.begin(), tempCache.end());
		}

		return mapCache;
	}

	void writeToCache(const wstring& key, const wstring& value) override {
		for (auto& cacher : _caches) {
			cacher.get().writeToCache(key, value);
		}
	}

	void writeAllToCache(const unordered_map<wstring, wstring> cache, bool reload = false) override {
		for (auto& cacher : _caches) {
			cacher.get().writeAllToCache(cache, reload);
		}
	}

	void removeFromCache(const wstring& key) override {
		for (auto& cacher : _caches) {
			cacher.get().removeFromCache(key);
		}
	}

	void clearCache() override {
		for (auto& cacher : _caches) {
			cacher.get().clearCache();
		}
	}
private:
	vector<reference_wrapper<TextMapCache>> _caches;
};
