
#pragma once
#include "../../Libraries/strhelper.h"
#include "../PathFormatter.h"
#include "../WinApiHelper.h"
#include "CustomPackageToModuleMapper.h"
#include "RequirementsTxtParser.h"
#include <functional>
#include <string>
#include <vector>
using namespace std;


class PipCmdStrBuilder {
public:
	virtual ~PipCmdStrBuilder() { }
	virtual string createInstallCmd(const string& packageName) = 0;
	virtual string createInstallCmd(const vector<string>& packageNames) = 0;
	virtual string createReqsTxtInstallCmd(const string& reqsTxtPath) = 0;
};


class DefaultPipCmdStrBuilder : public PipCmdStrBuilder {
public:
	DefaultPipCmdStrBuilder(const int pipPackageInstallMode = 0, const string& customPythonPath = "")
		: DefaultPipCmdStrBuilder([pipPackageInstallMode]() { return pipPackageInstallMode; }, 
			[customPythonPath]() { return customPythonPath; }) { }

	DefaultPipCmdStrBuilder(const function<int()>& pipPackageInstallModeGetter, 
		const function<string()>& customPythonPathGetter)
		: _pipPackageInstallModeGetter(pipPackageInstallModeGetter), 
			_customPythonPathGetter(customPythonPathGetter) { }

	string createInstallCmd(const vector<string>& packageNames) override {
		string pyModules = StrHelper::join<char>(",", _packageToModuleMapper.mapToModules(packageNames));
		string pipPackages = StrHelper::join<char>(" ", packageNames);
		return createInstallCmd(pyModules, "", pipPackages);
	}

	string createInstallCmd(const string& packageName) override {
		string moduleName = _packageToModuleMapper.mapToModule(packageName);
		return createInstallCmd(moduleName, "", packageName);
	}

	string createReqsTxtInstallCmd(const string& reqsTxtPath) override {
		string formattedReqsTxtPath = formatRequirementsTxtPath(reqsTxtPath);
		vector<string> reqsTxtPackages = _reqsTxtParser.getPackageNames(formattedReqsTxtPath);
		vector<string> reqsTxtModules = _packageToModuleMapper.mapToModules(reqsTxtPackages);

		return createInstallCmd(StrHelper::join<char>(",", reqsTxtModules), "-r", '"' + formattedReqsTxtPath + '"');
	}
private:
	DefaultPathFormatter _pathFormatter;
	DefaultCustomPackageToModuleMapper _packageToModuleMapper;
	DefaultRequirementsTxtParser _reqsTxtParser;
	const function<int()> _pipPackageInstallModeGetter;
	const function<string()> _customPythonPathGetter;

	const string _textFileExt = ".txt";
	const string _requirementsTxtFileName = "requirements" + _textFileExt;
	const string _nonExistDummyModule = "sdfkhasdfkjhasdfkjhasgdfkjhg";
	const string _upgradePackageOption = "-U";
	const string _reinstallPackageOption = "--force-reinstall";
	const string _cmdTemplate = "cmd /C \"{0}python -c \"import {1}\" 2>nul || {0}python -m pip install --disable-pip-version-check {2} {3} || (echo An unexpected error occurred while installing pip packages. && pause && exit /b 1)\"";
	// {0}: python path, {1}: comma-delimited python modules, {2}: additional pip options, {3}: main pip arg


	string createInstallCmd(string moduleList, string pipOptions, string pipMainArg) {
		string pyPath = _pathFormatter.formatDirPath(_customPythonPathGetter());
		if (!pyPath.empty()) pyPath = '"' + pyPath + '"';
		applyInstallModeToArgs(moduleList, pipOptions, pipMainArg);

		string command = StrHelper::format(_cmdTemplate, 
			vector<string>{ pyPath, moduleList, pipOptions, pipMainArg });

		return command;
	}

	void applyInstallModeToArgs(string& moduleList, string& pipOptions, string& pipMainArg) {
		int pipPackageInstallMode = _pipPackageInstallModeGetter();
		
		switch (pipPackageInstallMode) {
		case 1:
			pipOptions = _upgradePackageOption + " " + pipOptions;
			moduleList = _nonExistDummyModule;
			break;
		case 2:
			pipOptions = _reinstallPackageOption + " " + pipOptions;
			moduleList = _nonExistDummyModule;
			break;
		}
	}

	string formatRequirementsTxtPath(const string& requirementsTxtPath) {
		return requirementsTxtPath.rfind(_textFileExt) == string::npos ?
			_pathFormatter.formatDirPath(requirementsTxtPath) + _requirementsTxtFileName :
			requirementsTxtPath;
	}
};
