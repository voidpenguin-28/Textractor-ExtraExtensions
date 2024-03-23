
#pragma once
#include <string>
using namespace std;

class PathFormatter {
public:
	virtual ~PathFormatter() { }
	virtual string formatDirPath(string dirPath) = 0;
};


class DefaultPathFormatter : public PathFormatter {
public:
	virtual string formatDirPath(string dirPath) override {
		if (!dirPath.empty()) {
			if (dirPath.rfind('/') < dirPath.length() - 1) dirPath += '/';
			else if (dirPath.rfind('\\') != dirPath.length() - 1) dirPath += '\\';
		}

		return dirPath;
	}
};
