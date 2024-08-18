#pragma once

#include "Libraries/strhelper.h"
#include "python/WinApiHelper.h"
#include "Extension.h"
#include <functional>
#include <string>
#include <vector>
using namespace std;

class ScriptCmdStrHandler {
public:
	virtual ~ScriptCmdStrHandler() { }
	virtual string getPipRequirementsTxtPath() = 0;
	virtual string getOnScriptLoadCommand() = 0;
	virtual string getOnScriptUnloadCommand() = 0;
	virtual string getProcessSentenceDefinedCheckCommand() = 0;
	virtual string getOnProcessSentenceCommand(const string& sentence,
		SentenceInfoWrapper& sentenceInfo, bool returnErrMsg = false) = 0;
	virtual string parseCommandOutput(const string& fullOutput, const string& defaultValue) = 0;
	virtual bool containsErrMsg(const string& output) = 0;
};


class DefaultScriptCmdStrHandler : public ScriptCmdStrHandler {
public:
	DefaultScriptCmdStrHandler(const string& pipReqsTxtPath, const vector<pair<string, string>>& scriptCustomVars)
		: DefaultScriptCmdStrHandler([&pipReqsTxtPath]() { return pipReqsTxtPath; },
			[&scriptCustomVars]() { return scriptCustomVars; }) { }
	DefaultScriptCmdStrHandler(const function<string()>& pipReqsTxtPathGetter,
		const function<vector<pair<string, string>>()>& scriptCustomVarsGetter)
		: _pipReqsTxtPathGetter(pipReqsTxtPathGetter), _scriptCustomVarsGetter(scriptCustomVarsGetter) { }

	string getPipRequirementsTxtPath() override {
		return _pipReqsTxtPathGetter();
	}

	string getOnScriptLoadCommand() override {
		string customVarsJson = serializeJson(_scriptCustomVarsGetter(), true);
		return StrHelper::format(_onScriptLoadCommandTemplate, vector<string> { customVarsJson });
	}

	string getOnScriptUnloadCommand() override {
		string customVarsJson = serializeJson(_scriptCustomVarsGetter(), true);
		return StrHelper::format(_onScriptUnloadCommandTemplate, vector<string> { customVarsJson });
	}

	string getProcessSentenceDefinedCheckCommand() override {
		return _processSentenceDefinedCheckCommand;
	}

	string getOnProcessSentenceCommand(const string& sentence, 
		SentenceInfoWrapper& sentenceInfo, bool returnErrMsg = false) override
	{
		string command = returnErrMsg ? 
			_processSentenceCommandTryCatchTemplate : _processSentenceCommandTemplate;

		string sentenceInfoJson = serializeSentenceInfo(sentenceInfo);
		string customVarsJson = serializeJson(_scriptCustomVarsGetter(), true);

		return StrHelper::format(command, vector<string> { 
			formatSentence(sentence), sentenceInfoJson, customVarsJson
		});
	}

	string parseCommandOutput(const string& fullOutput, const string& defaultValue) override {
		string commandPrefix = getCommandPrefix();
		size_t prfxIndex = fullOutput.rfind(commandPrefix);
		if (prfxIndex == string::npos) return defaultValue;

		return fullOutput.substr(prfxIndex + commandPrefix.length());
	}

	bool containsErrMsg(const string& output) override {
		string errTag = getErrTag();
		return output.find(errTag) != string::npos;
	}
private:
	const function<string()> _pipReqsTxtPathGetter;
	const function<vector<pair<string, string>>()> _scriptCustomVarsGetter;
	const string _onScriptLoadCommandTemplate =
		createCommandStr("on_script_load({0}) if 'on_script_load' in globals() else True");
	const string _onScriptUnloadCommandTemplate =
		createCommandStr("on_script_unload({0}) if 'on_script_unload' in globals() else None", false);
	const string _processSentenceDefinedCheckCommand = createCommandStr("True if 'process_sentence' in globals() else False");
	const string _processSentenceCommandTemplate =
		createCommandStr("process_sentence({0}, {1}, {2})");
	const string _processSentenceCommandTryCatchTemplate =
		wrapCommandStrInTryCatch(_processSentenceCommandTemplate);
	
	static string getCommandPrefix() {
		static const string commandPrefix = "PyExtension-Output: ";
		return commandPrefix;
	}

	static string getErrTag() {
		static const string errTag = "[PYTHON ERROR]";
		return errTag;
	}

	static string createCommandStr(const string& rootCommand, bool printOutput = true) {
		string commandPrefix = getCommandPrefix();
		string command = printOutput ?
			"print('" + commandPrefix + "' + str(" + rootCommand + "))" :
			rootCommand;

		return command;
	}

	static string wrapCommandStrInTryCatch(const string& commandStr) {
		string commandPrefix = getCommandPrefix();
		string errTag = getErrTag();

		string command = "try:\n\t";
		command += commandStr;
		command += "\nexcept BaseException as ex:\n\t";
		command += "main_logger.error('An unhandled error has occurred.', exc_info=True)\n\t";
		command += "print('" + commandPrefix + "' + '" + errTag + " ' + str(ex))\n";

		return command;
	}

	string formatSentence(const string& sentence) {
		string formattedSent = StrHelper::replace<char>(sentence, "\\", "\\\\");
		formattedSent = StrHelper::replace<char>(sentence, "\"", "\\\"");

		return "\"\"\"" + formattedSent + "\"\"\"";
	}

	string serializeSentenceInfo(SentenceInfoWrapper& sentenceInfo) {
		const vector<pair<string, string>> sentPars = {
			pair<string, string>("current select", to_string(sentenceInfo.getCurrentSelect())),
			pair<string, string>("process id", to_string(sentenceInfo.getProcessId())),
			pair<string, string>("text number", to_string(sentenceInfo.getThreadNumber())),
			pair<string, string>("text name", '"' + sentenceInfo.getThreadName() + '"')
		};

		string json = serializeJson(sentPars, false);
		return json;
	}

	string serializeJson(const vector<pair<string, string>> vals, bool wrapValsInQuotes) {
		if (vals.empty()) return "{}";
		string json = "{";
		string val;

		for (const auto& par : vals) {
			val = par.second;
			if (wrapValsInQuotes) val = '"' + val + '"';

			json += createJsonKeyValue(par.first, val, true);
		}

		json[json.length() - 1] = '}';
		return json;
	}

	string createJsonKeyValue(const string& key, const string& value, bool appendComma) {
		string json = "\"" + key + "\":" + value;
		if (appendComma) json += ",";
		return json;
	}
};
