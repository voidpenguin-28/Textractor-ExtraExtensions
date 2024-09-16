
#pragma once
#include "Extension.h"
#include "Cache/TextMapCache.h"


// This class is meant to only ever store one key per thread.

class TextTempReadStore {
public:
	virtual ~TextTempReadStore() { }
	virtual pair<wstring, wstring> fromStore(SentenceInfoWrapper& sentInfoWrapper) const = 0;
};

class TextTempWriteStore {
public:
	virtual ~TextTempWriteStore() { }
	virtual void toStore(SentenceInfoWrapper& sentInfoWrapper, const wstring& key, const wstring& value) = 0;
	virtual void clearStore(SentenceInfoWrapper& sentInfoWrapper) = 0;
	virtual void clearStore() = 0;
};

class TextTempStore : public TextTempReadStore, public TextTempWriteStore {
public:
	virtual ~TextTempStore() { }
};


class NoTextTempStore : public TextTempStore {
public:
	pair<wstring, wstring> fromStore(SentenceInfoWrapper& sentInfoWrapper) const override {
		return pair<wstring, wstring>();
	}

	void toStore(SentenceInfoWrapper& sentInfoWrapper, const wstring& key, const wstring& value) override { }
	void clearStore(SentenceInfoWrapper& sentInfoWrapper) override { }
	void clearStore() override { }
};


// Stores key-value pair as a merged value to a map cache, under a temp key.
class MapCacheTextTempStore : public TextTempStore {
public:
	MapCacheTextTempStore(TextMapCache& cache, const TextMapper& mapper, 
		const wstring& cacheKeyBase) : _cache(cache), _mapper(mapper), _cacheKeyBase(cacheKeyBase) { }

	pair<wstring, wstring> fromStore(SentenceInfoWrapper& sentInfoWrapper) const override {
		wstring cacheKey = getCacheKey(sentInfoWrapper);
		wstring fullText = _cache.readFromCache(cacheKey);
		return _mapper.split(fullText);
	}

	void toStore(SentenceInfoWrapper& sentInfoWrapper, const wstring& key, const wstring& value) override {
		wstring cacheKey = getCacheKey(sentInfoWrapper);
		wstring fullText = _mapper.merge(key, value);
		clearStore(sentInfoWrapper);
		_cache.writeToCache(cacheKey, fullText);
	}

	void clearStore(SentenceInfoWrapper& sentInfoWrapper) override {
		wstring cacheKey = getCacheKey(sentInfoWrapper);
		_cache.removeFromCache(cacheKey);
	}

	void clearStore() override {
		_cache.clearCache();
	}
private:
	const wstring _cacheKeyBase;
	TextMapCache& _cache;
	const TextMapper& _mapper;

	wstring getCacheKey(SentenceInfoWrapper& sentInfoWrapper) const {
		return _cacheKeyBase + sentInfoWrapper.getThreadNumberW();
	}
};
