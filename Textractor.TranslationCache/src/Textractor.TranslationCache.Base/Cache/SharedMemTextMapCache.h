
#pragma once
#include "TextMapCache.h"
#include "../SharedMemory/SharedMemoryManager.h"
#include <unordered_map>


class SharedMemTextMapCache : public TextMapCache {
public:
	SharedMemTextMapCache(const TextFormatter& formatter, SharedMemoryManager& sharedMemManager,
		const bool deleteInstanceOnKeyRemoval) : _formatter(formatter), _sharedMemManager(sharedMemManager), 
		_deleteInstanceOnKeyRemoval(deleteInstanceOnKeyRemoval) { }

	virtual ~SharedMemTextMapCache() {
		_sharedMemMap.clear();
	}

	bool keyExists(const wstring& key) const override {
		if (_deleteInstanceOnKeyRemoval) {
			wstring formattedKey = _formatter.format(key);
			return _sharedMemManager.instanceExists(formattedKey);
		}

		return !readFromCache(key).empty();
	}

	wstring readFromCache(const wstring& key) const override {
		wstring formattedKey = _formatter.format(key);
		if (!_sharedMemManager.instanceExists(formattedKey)) return L"";

		shared_ptr<SharedMemoryInstance> sharedMem = createOrGetSharedMemInstance(formattedKey);
		return sharedMem->readData();
	}

	unordered_map<wstring, wstring> readAllFromCache() const override {
		unordered_map<wstring, wstring> cache{};
		wstring value;
		
		for (const auto& sharedMem : _sharedMemMap) {
			value = readFromCache(sharedMem.first);
			if(!value.empty()) cache[sharedMem.first] = value;
		}

		return cache;
	}

	void writeToCache(const wstring& key, const wstring& value) override {
		wstring formattedKey = _formatter.format(key);
		shared_ptr<SharedMemoryInstance> sharedMem = createOrGetSharedMemInstance(formattedKey);
		sharedMem->writeData(value);
	}

	void writeAllToCache(const unordered_map<wstring, wstring> cache, bool reload = false) override {
		if (reload) clearCache();

		for (const auto& keyVal : cache) {
			writeToCache(keyVal.first, keyVal.second);
		}
	}

	void removeFromCache(const wstring& key) override {
		removeFromCache(key, _deleteInstanceOnKeyRemoval);
	}

	void clearCache() override {
		clearCache(_deleteInstanceOnKeyRemoval);
	}
private:
	mutable BasicLocker _memMapLocker;
	const TextFormatter& _formatter;
	SharedMemoryManager& _sharedMemManager;
	const bool _deleteInstanceOnKeyRemoval;
	mutable unordered_map<wstring, shared_ptr<SharedMemoryInstance>> _sharedMemMap{};

	shared_ptr<SharedMemoryInstance> createOrGetSharedMemInstance(const wstring& key) const {
		shared_ptr<SharedMemoryInstance> instance;
		
		_memMapLocker.lock([this, &key, &instance]() {
			if (!sharedMemMapContains(key))
				_sharedMemMap[key] = _sharedMemManager.createOrOpenInstance(key);

			instance = _sharedMemMap.at(key);
		});

		return instance;
	}

	bool sharedMemMapContains(const wstring& key) const {
		return _sharedMemMap.find(key) != _sharedMemMap.end();
	}

	void removeFromCache(const wstring& key, bool removeFromMap) {
		wstring formattedKey = _formatter.format(key);
		if (!_sharedMemManager.instanceExists(formattedKey)) return;

		shared_ptr<SharedMemoryInstance> sharedMem = createOrGetSharedMemInstance(formattedKey);
		sharedMem->writeData(L"");
		if (removeFromMap) removeFromSharedMemMap(formattedKey);
	}

	void clearCache(bool removeFromMap) {
		for (const auto& sharedMem : _sharedMemMap) {
			removeFromCache(sharedMem.first, removeFromMap);
		}
	}

	void removeFromSharedMemMap(const wstring& key) {
		_memMapLocker.lock([this, &key]() {
			if (!sharedMemMapContains(key)) return;
			_sharedMemMap.erase(key);
		});
	}
};
