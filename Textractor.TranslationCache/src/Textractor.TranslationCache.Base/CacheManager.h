
#pragma once
#include "Extension.h"
#include "ExtensionConfig.h"
#include "ExtExecRequirements.h"
#include "TextTempStore.h"
#include "Cache/TextMapCache.h"


class CacheManager {
public:
	virtual ~CacheManager() { }
	virtual wstring readCacheAndStoreTemp(wstring& sentence, 
		SentenceInfoWrapper& sentInfoWrapper, const ExtensionConfig& config) = 0;
	virtual wstring writeCacheOrLoadTemp(wstring& sentence, 
		SentenceInfoWrapper& sentInfoWrapper, const ExtensionConfig& config) = 0;
	virtual void clearCache() = 0;
};


class DefaultCacheManager : public CacheManager {
public:
	DefaultCacheManager(TextMapCache& cache, TextTempStore& tempStore, 
		const TextMapper& textractorTextMapper, const ExtExecRequirements& execRequirements,
		const bool updateCacheFromTempStoreBeforeRead) : _cache(cache), _tempStore(tempStore), 
			_textractorTextMapper(textractorTextMapper), _execRequirements(execRequirements), 
			_updateCacheFromTempStoreBeforeRead(updateCacheFromTempStoreBeforeRead) { }

	wstring readCacheAndStoreTemp(wstring& sentence,
		SentenceInfoWrapper& sentInfoWrapper, const ExtensionConfig& config) override
	{
		if (!_execRequirements.meetsRequirements(sentInfoWrapper, config, true)) return sentence;

		// if the writer wrote a value to cache, we may need to update the 
		// reader cache to contain that new value by retrieving it from the temp store
		if (_updateCacheFromTempStoreBeforeRead) {
			pair<wstring, wstring> mappingToCache = _tempStore.fromStore(sentInfoWrapper);
			if (validMapping(mappingToCache)) {
				_cache.writeToCache(mappingToCache.first, mappingToCache.second);
				_tempStore.clearStore(sentInfoWrapper);
			}
		}

		wstring transText = _cache.readFromCache(sentence);
		if (transText.empty()) return sentence;

		_tempStore.toStore(sentInfoWrapper, sentence, transText);
		return applySkippingStrategy(config.skippingStrategy, sentence);
	}

	wstring writeCacheOrLoadTemp(wstring& sentence,
		SentenceInfoWrapper& sentInfoWrapper, const ExtensionConfig& config) override
	{
		if (!_execRequirements.meetsRequirements(sentInfoWrapper, config, false)) return sentence;
		pair<wstring, wstring> textMapping = _tempStore.fromStore(sentInfoWrapper);

		if (validMapping(textMapping)) {
			sentence = _textractorTextMapper.merge(textMapping.first, textMapping.second);
			if (config.debugMode) sentence = L"FROM CACHE:\n" + sentence;

			_tempStore.clearStore(sentInfoWrapper);
			return sentence;
		}

		textMapping = _textractorTextMapper.split(sentence);

		if (shouldWriteToCache(config, textMapping)) {
			_cache.writeToCache(textMapping.first, textMapping.second);
			_tempStore.toStore(sentInfoWrapper, textMapping.first, textMapping.second);
		}
		
		return sentence;
	}

	void clearCache() override {
		_cache.clearCache();
	}
private:
	const wstring _largeText = wstring(9999, L'a') + L'.';
	const wstring _dummyText = L"TRANS_CACHE_DUMMY_VAL";
	const wstring _zeroWidthSpace = L"\x200b";
	const wstring _zeroWidthSpaceDummyText = _dummyText + _zeroWidthSpace + _dummyText;

	TextMapCache& _cache;
	TextTempStore& _tempStore;
	const TextMapper& _textractorTextMapper;
	const ExtExecRequirements& _execRequirements;
	const bool _updateCacheFromTempStoreBeforeRead;

	bool validMapping(const pair<wstring, wstring>& mapping) {
		return !mapping.first.empty() && !mapping.second.empty();
	}

	wstring applySkippingStrategy(const ExtensionConfig::SkippingStrategy strategy, const wstring& sentence) {
		switch (strategy) {
		case ExtensionConfig::SkippingStrategy::SendZeroWidthSpace:
			return _zeroWidthSpaceDummyText;
		case ExtensionConfig::SkippingStrategy::ExceedTextLengthLimit:
			return _largeText;
		case ExtensionConfig::SkippingStrategy::SendDummyText:
			return _dummyText;
		default:
			throw runtime_error("No implementation exists for SkippingStrategy: " + to_string(strategy));
		}
	}

	bool shouldWriteToCache(const ExtensionConfig& config, const pair<wstring, wstring>& textMapping) const {
		if (config.disabledMode == ExtensionConfig::DisableWrite) return false;
		if (textMapping.second.empty()) return false;
		if (textMapping.first.size() + textMapping.second.size() > (size_t)config.cacheLineLengthLimit) return false;

		return true;
	}
};
