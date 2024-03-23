
#pragma once
#include <condition_variable>
#include <functional>
#include <mutex>
using namespace std;

class Locker {
public:
	virtual ~Locker() { }
	virtual bool tryLock(const function<void()>& action) = 0;
	virtual void lock(const function<void()>& action) = 0;
	virtual void waitForUnlock() = 0;
};


class BasicLocker : public Locker {
public:
	bool tryLock(const function<void()>& action) override {
		if (!_mtx.try_lock()) return false;

		{
			lock_guard<mutex> lock(_mtx, adopt_lock);
			performAction(action);
		}

		_cv.notify_all();
		return true;
	}

	void lock(const function<void()>& action) override {
		{
			lock_guard<mutex> lock(_mtx);
			performAction(action);
		}

		_cv.notify_all();
	}

	void waitForUnlock() override {
		unique_lock<mutex> lock(_mtx);
		_cv.wait(lock, [this]() { return !_busy; });
	}
private:
	mutex _mtx;
	condition_variable _cv;
	bool _busy = false;

	void performAction(const function<void()>& action) {
		_busy = true;

		try {
			action();
			_busy = false;
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
		{
			unique_lock<mutex> lock(_mtx);
			if (allTaken()) return false;

			waitForUnlock(lock);
			_takenCount++;
		}

		performAction(action);
		return true;
	}

	void lock(const function<void()>& action) override {
		{
			unique_lock<mutex> lock(_mtx);
			waitForUnlock(lock);
			_takenCount++;
		}

		performAction(action);
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

	bool allTaken() {
		return _takenCount == _maxCount;
	}

	void performAction(const function<void()>& action) {
		try {
			action();
			_takenCount--;
			_cv.notify_all();
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
