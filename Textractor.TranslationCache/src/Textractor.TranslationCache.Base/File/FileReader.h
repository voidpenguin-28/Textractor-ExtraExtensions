
#pragma once
#include <fstream>
#include <functional>
#include <sstream>
#include <vector>
using namespace std;


class FileReader {
public:
	virtual ~FileReader() { }
	virtual bool fileExists(const string& filePath) = 0;
	virtual vector<string> readLines(const string& filePath) = 0;
	virtual void readLines(const string& filePath, const function<void(const string& line)>& lineAction) = 0;
	virtual string readLine(const string& filePath, size_t i) = 0;
	virtual string readLine(const string& filePath, const function<bool(const string& line)> condition) = 0;
	virtual string readAll(const string& filePath) = 0;
};


class NoFileReader : public FileReader {
public:
	bool fileExists(const string& filePath) override { return false; }
	vector<string> readLines(const string& filePath) override { return vector<string>{}; };
	void readLines(const string& filePath, const function<void(const string& line)>& lineAction) override { }
	string readLine(const string& filePath, size_t i) override { return ""; }
	string readLine(const string& filePath, const function<bool(const string& line)> condition) override { return ""; }
	string readAll(const string& filePath) override { return ""; }
};


class FstreamFileReader : public FileReader {
public:
	bool fileExists(const string& filePath) override {
		ifstream f(filePath, ios_base::in);
		return fileExists(f);
	}

	vector<string> readLines(const string& filePath) override {
		vector<string> lines{};
		readLines(filePath, [&lines](const string& line) { lines.push_back(line); });
		return lines;
	}

	void readLines(const string& filePath, const function<void(const string& line)>& lineAction) override {
		readLine(filePath, [&lineAction](const string& line) {
			lineAction(line);
			return false;
		});
	}

	string readLine(const string& filePath, size_t i) override {
		size_t currI = 0;

		return readLine(filePath, [i, &currI](const string& line) {
			return i == currI++;
		});
	}

	string readLine(const string& filePath, const function<bool(const string& line)> condition) override {
		ifstream f(filePath, ios_base::in);
		if (!fileExists(f)) return "";
		char* line = new char[MAX_LENGTH];

		while (f.getline(line, MAX_LENGTH)) {
			if (condition(line)) return line;
		}

		return "";
	}

	string readAll(const string& filePath) override {
		ifstream f(filePath, ios_base::in);
		if (!fileExists(f)) return "";

		stringstream buffer;
		buffer << f.rdbuf();
		return buffer.str();
	}
private:
	static const int MAX_LENGTH = 524288;

	bool fileExists(const ifstream& f) const {
		return f.good();
	}
};
