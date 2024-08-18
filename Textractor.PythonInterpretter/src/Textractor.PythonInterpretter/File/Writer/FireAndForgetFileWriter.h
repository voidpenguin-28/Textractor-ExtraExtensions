
#pragma once
#include "FileWriter.h"
#include <atomic>
#include <functional>
#include <thread>
#include <windows.h>


class FireAndForgetFileWriter : public FileWriter {
public:
	FireAndForgetFileWriter(FileWriter& mainWriter) : _mainWriter(mainWriter) { }

	~FireAndForgetFileWriter() {
		// ensure all ongoing write activity is done before deallocating
		_destrCalled = true;
		waitForAllActionsDone(100, 5000);
	}

	void writeToFile(const string& filePath, const string& text) override {
		thread([this, filePath, text]() { writeToFileBase(filePath, text); }).detach();
	}

	void writeToFile(const string& filePath, const vector<string>& lines, size_t startIndex = 0) override {
		thread([this, filePath, lines, startIndex]() {
			writeToFileBase(filePath, lines, startIndex); }).detach();
	}

	void appendToFile(const string& filePath, const string& text) override {
		thread([this, filePath, text]() { appendToFileBase(filePath, text); }).detach();
	}

	void appendToFile(const string& filePath, const vector<string>& lines, size_t startIndex = 0) override {
		thread([this, filePath, lines, startIndex]() {
			appendToFileBase(filePath, lines, startIndex); }).detach();
	}
private:
	atomic<size_t> _activeCount = 0;
	atomic_bool _destrCalled = false;
	FileWriter& _mainWriter;

	void writeToFileBase(const string& filePath, const string& text) {
		trackAction([this, &filePath, &text]() {
			_mainWriter.writeToFile(filePath, text);
		});
	}

	void writeToFileBase(const string& filePath, const vector<string>& lines, size_t startIndex = 0) {
		trackAction([this, &filePath, &lines, startIndex]() {
			_mainWriter.writeToFile(filePath, lines, startIndex);
		});
	}

	void appendToFileBase(const string& filePath, const string& text) {
		trackAction([this, &filePath, &text]() {
			_mainWriter.appendToFile(filePath, text);
		});
	}

	void appendToFileBase(const string& filePath, const vector<string>& lines, size_t startIndex = 0) {
		trackAction([this, &filePath, &lines, startIndex]() {
			_mainWriter.appendToFile(filePath, lines, startIndex);
		});
	}

	void trackAction(const function<void()>& action) {
		if (_destrCalled) return;
		_activeCount++;

		try {
			action();
			_activeCount--;
		}
		catch (const exception&) {
			_activeCount--;
			throw;
		}
	}

	void waitForAllActionsDone(DWORD intervalMs, DWORD maxWaitMs) {
		DWORD waitMs = 0;

		while (_activeCount > 0 && waitMs < maxWaitMs) {
			Sleep(intervalMs);
			waitMs += intervalMs;
		}
	}
};
