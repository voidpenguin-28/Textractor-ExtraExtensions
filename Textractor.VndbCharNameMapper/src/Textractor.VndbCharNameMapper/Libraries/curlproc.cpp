
#include "curlproc.h"
#include <windows.h>
#include <stdexcept>
#include <codecvt>


namespace curlproc {
#define BUFSIZE 4096
	string buildCurlCommand(const string& url, const string& method = "GET", const string& postData = "", const vector<string>& headers = vector<string>(), const string& userAgent = "", const string& customCurlPath = "");
	string formatPath(string path);
	string replace(const string& input, const string& target, const string& replacement);
	string runProcess(const string& command);
    wstring convertToW(const string& str);


    string httpGet(const string& url, const vector<string>& headers, const string& userAgent, const string& customCurlPath) {
		string command = buildCurlCommand(url, "GET", "", headers, userAgent, customCurlPath);
		string output = runProcess(command);
		return output;
	}

    string httpPost(const string& url, const string& body, const vector<string>& headers, const string& userAgent, const string& customCurlPath) {
		string command = buildCurlCommand(url, "POST", body, headers, userAgent, customCurlPath);
		string output = runProcess(command);
		return output;
	}

    string buildCurlCommand(const string& url, const string& method, const string& postData, const vector<string>& headers, const string& userAgent, const string& customCurlPath) {
		string command = formatPath(customCurlPath);
		command += "curl -L -s";

		if (method.length() > 0) command += " -X " + method;
        command += " -m 10";

        for (size_t i = 0; i < headers.size(); i++) {
            if (headers[i].length() == 0) continue;
            command += " -H \"" + headers[i] + "\"";
        }

		if (userAgent.length() > 0) command += " -A \"" + userAgent + "\"";
		if (postData.length() > 0) command += " -d \"" + replace(postData, "\"", "\\\"") + "\"";

		command += " " + url;
		return command;
	}

	string formatPath(string path) {
		if (path.length() == 0) return path;
		char pathEndCh = path[path.length() - 1];

		if (pathEndCh != '/' && path.find('/') != string::npos)
			path += '/';
		else if (pathEndCh != '\\')
			path += '\\';

		return path;
	}

	string replace(const string& input, const string& target, const string& replacement) {
		string result = input;
		size_t startPos = 0;

		while ((startPos = result.find(target, startPos)) != string::npos) {
			result.replace(startPos, target.length(), replacement);
			startPos += replacement.length();
		}

		return result;
	}

	string runProcess(const string& command) {
        string outOutput, errOutput;
        HANDLE g_hChildStd_OUT_Rd = NULL;
        HANDLE g_hChildStd_OUT_Wr = NULL;
        HANDLE g_hChildStd_ERR_Rd = NULL;
        HANDLE g_hChildStd_ERR_Wr = NULL;
        SECURITY_ATTRIBUTES sa;

        // Set the bInheritHandle flag so pipe handles are inherited.
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = NULL;
        if (!CreatePipe(&g_hChildStd_ERR_Rd, &g_hChildStd_ERR_Wr, &sa, 0)) // Create a pipe for the child process's STDERR.
            throw runtime_error("Failed to create pipe for STDERR. Command: " + command);
        if (!SetHandleInformation(g_hChildStd_ERR_Rd, HANDLE_FLAG_INHERIT, 0)) // Ensure the read handle to the pipe for STDERR is not inherited.
            throw runtime_error("Read handel for STDERR pipe should not be inherited. Command: " + command);
        if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &sa, 0)) // Create a pipe for the child process's STDOUT.
            throw runtime_error("Failed to create pipe for STDOUT. Command: " + command);
        if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) // Ensure the read handle to the pipe for STDOUT is not inherited
            throw runtime_error("Read handel for STDOUT pipe should not be inherited. Command: " + command);

        PROCESS_INFORMATION piProcInfo;
        STARTUPINFO siStartInfo;

        // Set up members of the PROCESS_INFORMATION structure.
        ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

        // Set up members of the STARTUPINFO structure.
        // This structure specifies the STDERR and STDOUT handles for redirection.
        ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
        siStartInfo.cb = sizeof(STARTUPINFO);
        siStartInfo.hStdError = g_hChildStd_ERR_Wr;
        siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

        // Create the child process.
        bool bSuccess = CreateProcess(
            NULL,             // program name
            (wchar_t*)(convertToW(command).c_str()),       // command line
            NULL,             // process security attributes
            NULL,             // primary thread security attributes
            TRUE,             // handles are inherited
            CREATE_NO_WINDOW, // creation flags (this is what hides the window)
            NULL,             // use parent's environment
            NULL,             // use parent's current directory
            &siStartInfo,     // STARTUPINFO pointer
            &piProcInfo       // receives PROCESS_INFORMATION
        );

        if (!bSuccess)
            throw runtime_error("Process failed to run for command. Command: " + command);

        CloseHandle(g_hChildStd_ERR_Wr);
        CloseHandle(g_hChildStd_OUT_Wr);

        // read output
        DWORD dwRead;
        CHAR chBuf[BUFSIZE];
        bool bSuccess2 = FALSE;

        for (;;) { // read stdout
            bSuccess2 = ReadFile(g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
            if (!bSuccess2 || dwRead == 0) break;
            string s(chBuf, dwRead);
            outOutput += s;
        }

        dwRead = 0;

        for (;;) { // read stderr
            bSuccess2 = ReadFile(g_hChildStd_ERR_Rd, chBuf, BUFSIZE, &dwRead, NULL);
            if (!bSuccess2 || dwRead == 0) break;
            string s(chBuf, dwRead);
            errOutput += s;
        }

        CloseHandle(g_hChildStd_ERR_Rd);
        CloseHandle(g_hChildStd_OUT_Rd);
        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);

        if (errOutput.length() > 0)
            throw runtime_error("Running command resulted in error message: " + errOutput + "\nCommand: " + command);

        return outOutput;
	}

    wstring convertToW(const string& str) {
        return wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().from_bytes(str);
    }

}
