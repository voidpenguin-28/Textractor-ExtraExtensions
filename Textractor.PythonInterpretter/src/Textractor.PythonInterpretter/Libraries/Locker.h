
#pragma once
#include <condition_variable>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <windows.h>
using namespace std;

class Locker {
public:
	virtual ~Locker() { }
	virtual bool tryLock(const function<void()>& action) = 0;
	virtual bool tryLockS(const function<string()>& action, string& output) = 0;
	virtual bool tryLockWS(const function<wstring()>& action, wstring& output) = 0;
	virtual bool tryLockI(const function<int()>& action, int& output) = 0;
	virtual bool tryLockB(const function<bool()>& action, bool& output) = 0;
	virtual bool tryLockD(const function<double()>& action, double& output) = 0;
	virtual bool tryLockDW(const function<DWORD()>& action, DWORD& output) = 0;

	virtual void lock(const function<void()>& action) = 0;
	virtual string lockS(const function<string()>& action) = 0;
	virtual wstring lockWS(const function<wstring()>& action) = 0;
	virtual bool lockB(const function<bool()>& action) = 0;
	virtual int lockI(const function<int()>& action) = 0;
	virtual double lockD(const function<double()>& action) = 0;
	virtual DWORD lockDW(const function<DWORD()>& action) = 0;

	virtual void waitForUnlock() = 0;
};


class BasicLocker : public Locker {
public:
	bool tryLock(const function<void()>& action) override {
		bool dummy;
		return tryLock<bool>([&action]() {
			action();
			return true;
			}, dummy);
	}

	bool tryLockS(const function<string()>& action, string& output) override {
		return tryLock<string>(action, output);
	}

	bool tryLockWS(const function<wstring()>& action, wstring& output) {
		return tryLock<wstring>(action, output);
	}

	bool tryLockB(const function<bool()>& action, bool& output) override {
		return tryLock<bool>(action, output);
	}

	bool tryLockI(const function<int()>& action, int& output) override {
		return tryLock<int>(action, output);
	}

	bool tryLockD(const function<double()>& action, double& output) override {
		return tryLock<double>(action, output);
	}

	bool tryLockDW(const function<DWORD()>& action, DWORD& output) override {
		return tryLock<DWORD>(action, output);
	}

	void lock(const function<void()>& action) override {
		lock<bool>([&action]() {
			action();
			return true;
			});
	}

	string lockS(const function<string()>& action) override {
		return lock<string>(action);
	}

	wstring lockWS(const function<wstring()>& action) override {
		return lock<wstring>(action);
	}

	bool lockB(const function<bool()>& action) override {
		return lock<bool>(action);
	}

	int lockI(const function<int()>& action) override {
		return lock<int>(action);
	}

	double lockD(const function<double()>& action) override {
		return lock<double>(action);
	}

	DWORD lockDW(const function<DWORD()>& action) override {
		return lock<DWORD>(action);
	}

	void waitForUnlock() override {
		unique_lock<mutex> lock(_mtx);
		_cv.wait(lock, [this]() { return !_busy; });
	}
private:
	mutex _mtx;
	condition_variable _cv;
	bool _busy = false;

	template<typename T>
	bool tryLock(const function<T()>& action, T& output) {
		if (!_mtx.try_lock()) return false;

		{
			lock_guard<mutex> lock(_mtx, adopt_lock);
			output = performAction(action);
		}

		_cv.notify_all();
		return true;
	}

	template<typename T>
	T lock(const function<T()>& action) {
		T output;
		{
			lock_guard<mutex> lock(_mtx);
			output = performAction(action);
		}

		_cv.notify_all();
		return output;
	}

	template<typename T>
	T performAction(const function<T()>& action) {
		_busy = true;

		try {
			T output = action();
			_busy = false;
			return output;
		}
		catch (exception&) {
			_busy = false;
			_cv.notify_all();
			throw;
		}
	}
};


class SemaphoreLocker : public Locker {
public:
	SemaphoreLocker(int maxCount = MAXINT) : _maxCount(maxCount) { }

	bool tryLock(const function<void()>& action) override {
		bool dummy;
		return tryLock<bool>([&action]() {
			action();
			return true;
			}, dummy);
	}

	bool tryLockS(const function<string()>& action, string& output) override {
		return tryLock<string>(action, output);
	}

	bool tryLockWS(const function<wstring()>& action, wstring& output) override {
		return tryLock<wstring>(action, output);
	}

	bool tryLockB(const function<bool()>& action, bool& output) override {
		return tryLock<bool>(action, output);
	}

	bool tryLockI(const function<int()>& action, int& output) override {
		return tryLock<int>(action, output);
	}

	bool tryLockD(const function<double()>& action, double& output) override {
		return tryLock<double>(action, output);
	}

	bool tryLockDW(const function<DWORD()>& action, DWORD& output) override {
		return tryLock<DWORD>(action, output);
	}

	void lock(const function<void()>& action) override {
		lock<bool>([&action]() {
			action();
			return true;
			});
	}

	string lockS(const function<string()>& action) override {
		return lock<string>(action);
	}

	wstring lockWS(const function<wstring()>& action) override {
		return lock<wstring>(action);
	}

	bool lockB(const function<bool()>& action) override {
		return lock<bool>(action);
	}

	int lockI(const function<int()>& action) override {
		return lock<int>(action);
	}

	double lockD(const function<double()>& action) override {
		return lock<double>(action);
	}

	DWORD lockDW(const function<DWORD()>& action) override {
		return lock<DWORD>(action);
	}

	void waitForUnlock() override {
		unique_lock<mutex> lock(_mtx);
		waitForUnlock(lock);
	}

	void waitForAllUnlocked() {
		unique_lock<mutex> lock(_mtx);
		waitForAllUnlocked(lock);
	}
private:
	mutex _mtx;
	condition_variable _cv;
	int _maxCount;
	atomic<int> _takenCount = 0;

	template<typename T>
	bool tryLock(const function<T()>& action, T& output) {
		{
			unique_lock<mutex> lock(_mtx);
			if (allTaken()) return false;

			waitForUnlock(lock);
			_takenCount++;
		}

		output = performAction(action);
		return true;
	}

	bool allTaken() {
		return _takenCount == _maxCount;
	}

	template<typename T>
	T lock(const function<T()>& action) {
		{
			unique_lock<mutex> lock(_mtx);
			waitForUnlock(lock);
			_takenCount++;
		}

		return performAction(action);
	}

	template<typename T>
	T performAction(const function<T()>& action) {
		try {
			T output = action();
			_takenCount--;
			_cv.notify_all();
			return output;
		}
		catch (exception&) {
			_takenCount--;
			_cv.notify_all();
			throw;
		}
	}

	void waitForUnlock(unique_lock<mutex>& lock) {
		_cv.wait(lock, [this]() { return _takenCount < _maxCount; });
	}

	void waitForAllUnlocked(unique_lock<mutex>& lock) {
		_cv.wait(lock, [this]() { return _takenCount == 0; });
	}
};



template<typename T>
class LockerMap {
public:
	virtual Locker& getOrCreateLocker(const T& key) = 0;
};


template<typename T>
class DefaultLockerMap : public LockerMap<T> {
public:
	DefaultLockerMap(const function<Locker* ()>&
		lockerCreator = getDefaultLockerCreator()) : _lockerCreator(lockerCreator) { }

	~DefaultLockerMap() {
		_lockerMap.clear();
	}

	Locker& getOrCreateLocker(const T& key) override {
		Locker* locker = nullptr;

		_mainLocker.lock([this, &key, &locker]() {
			if (_lockerMap.find(key) == _lockerMap.end())
				_lockerMap[key] = unique_ptr<Locker>(_lockerCreator());

			locker = _lockerMap[key].get();
			});

		return *locker;
	}
private:
	BasicLocker _mainLocker;
	unordered_map<T, unique_ptr<Locker>> _lockerMap{};
	const function<Locker* ()> _lockerCreator;

	static const function<Locker* ()> getDefaultLockerCreator() {
		static const function<Locker* ()> _defaultLockerCreator = []() { return new BasicLocker(); };
		return _defaultLockerCreator;
	}

	bool containsLocker(const T& key) const {
		return _lockerMap.find(key) != _lockerMap.end();
	}
};
