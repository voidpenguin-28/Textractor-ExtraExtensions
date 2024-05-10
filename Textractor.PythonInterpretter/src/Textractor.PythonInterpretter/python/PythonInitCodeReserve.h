
#pragma once
#include "PathFormatter.h"
#include "WinApiHelper.h"
#include <cctype>
#include <fstream>
#include <string>
#include <vector>
using namespace std;


class PythonInitCodeReserve {
public:
    struct CmdPars {
        const string rootName;
        const size_t bufferSize;
        const int showLogConsole;
        const Logger::Level logLevel;

        CmdPars(const string& rootName_, const size_t bufferSize_, 
            const int showLogConsole_, const Logger::Level logLevel_)
            : rootName(rootName_), bufferSize(bufferSize_), showLogConsole(showLogConsole_), logLevel(logLevel_) { }
    };

    virtual ~PythonInitCodeReserve() { }

    const string PyNoData = "NULL";
    const string ManagerPipeId = "Manager";
    const string StopCmd = "```STOP```";
    const string ForceStopCmd = "```FORCE_STOP```";

    virtual void exportInitScripts(const string& id) const = 0;
    virtual void removeInitScripts(const string& id) const = 0;

    virtual string createStartCmd(const string& id, const CmdPars& pars) const = 0;
    virtual string createCmdPiperStartCmd(const string& id, const size_t bufferSizeOverride = 0) const = 0;
    virtual string createSetLogLevelCmd(const string& id, Logger::Level logLevel) const = 0;
    virtual vector<string> getRequiredPipPackages() const = 0;
};


class DefaultPythonInitCodeReserve : public PythonInitCodeReserve {
public:
    DefaultPythonInitCodeReserve(const string& scriptDirPath, 
        const string& logDirPath, const string& customPythonPath = "")
        : _scriptDirPath(_pathFormatter.formatDirPath(scriptDirPath)), 
            _logDirPath(_pathFormatter.formatDirPath(logDirPath)),
            _customPythonPath(_pathFormatter.formatDirPath(customPythonPath)) { }

    void exportInitScripts(const string& id) const override {
        string libCode = getCmdPiperLibCode();
        string mainCode = getMainExecCode(id);

        string libScriptPath = getLibScriptPath(id);
        string mainScriptPath = getMainScriptPath(id);

        writeToFile(libScriptPath, libCode);
        writeToFile(mainScriptPath, mainCode);
    }

    void removeInitScripts(const string& id) const override {
        string libScriptPath = getLibScriptPath(id);
        string mainScriptPath = getMainScriptPath(id);

        deleteFile(mainScriptPath);
        deleteFile(libScriptPath);
    }

    string createStartCmd(const string& id, const CmdPars& pars) const override {
        string mainScriptPath = getMainScriptPath(id);
        string logFilePath = getLogFilePath(id);
        string initFatalLogFilePath = getInitFatalLogFilePath(id);
        int pyLogLevel = toPyLogLevel(pars.logLevel);

        return "cmd /C \"\"" 
            + _customPythonPath + "python\" "
            + quoteWrap(mainScriptPath) + " "
            + quoteWrap(pars.rootName) + " "
            + quoteWrap(to_string(pars.bufferSize)) + " "
            + quoteWrap(to_string(pars.showLogConsole)) + " "
            + quoteWrap(to_string(pyLogLevel)) + " "
            + quoteWrap(logFilePath)
            + " 2>> \"" + initFatalLogFilePath + "\"\"";
    }
    
    string createCmdPiperStartCmd(const string& id, const size_t bufferSizeOverride = 0) const override {
        string command = "___piper_manager.start_piper('" + id + "'";
        if (bufferSizeOverride > 0) command += ", " + bufferSizeOverride;
        command += ")";

        return command;
    }

    string createSetLogLevelCmd(const string& id, Logger::Level logLevel) const override {
        static const string command = "main_logger.setLevel({0})";
        int pyLogLevel = toPyLogLevel(logLevel);
        return WinApiHelper::replace(command, "{0}", to_string(pyLogLevel));
    }

    vector<string> getRequiredPipPackages() const override {
        return _requiredPipPackages;
    }
private:
    DefaultPathFormatter _pathFormatter;
    const string _managerPipeId = ManagerPipeId;
    const vector<string> _requiredPipPackages = { "pypiwin32" };
    const string _scriptDirPath;
    const string _logDirPath;
    const string _customPythonPath;

    string quoteWrap(const string& str) const {
        return "\"" + str + "\"";
    }

    int toPyLogLevel(Logger::Level logLevel) const {
        return (logLevel + 1) * 10;
    }

    string getLibScriptPath(const string& id) const {
        string moduleName = idToModuleName(id);
        return createScriptPath(moduleName);
    }

    string getMainScriptPath(const string& id) const {
        string name = getMainScriptName(id);
        return createScriptPath(name);
    }

    string getMainScriptName(const string& id) const {
        return id + "-client";
    }

    string createScriptPath(const string& name) const {
        return _scriptDirPath + name + ".py";
    }

    string getLogFilePath(const string& id) const {
        string name = getMainScriptName(id);
        return _logDirPath + name + "-log.txt";
    }

    string getInitFatalLogFilePath(const string& id) const {
        string name = getMainScriptName(id);
        return _logDirPath + name + "-init-fatal-log.txt";
    }

    void writeToFile(const string& filePath, const string& str) const {
        ofstream file(filePath);
        file << str;
        file.close();
    }

    void deleteFile(const string& filePath) const {
        remove(filePath.c_str());
    }
    
    static string idToModuleName(const string& id) {
        static constexpr char DEFAULT_CH = '_';
        if (id.empty()) return id;

        string newId = "";
        if (isdigit(id[0])) newId += DEFAULT_CH;

        for (char ch : id) {
            newId += ispunct(ch) ? DEFAULT_CH : ch;
        }

        return newId;
    }

    string getCmdPiperLibCode() const {
        static const string _initPythonTemplate1 = R"(

import sys, os, locale
import threading
import win32pipe, win32file, win32event, pywintypes
from subprocess import Popen, PIPE, CREATE_NEW_CONSOLE
from io import StringIO
import time
import logging
from logging.handlers import RotatingFileHandler


class WinErr: #{
    FILE_NOT_FOUND = 2
    INVALID_HANDLE = 6
    INVALID_PARAMETER = 87
    BROKEN_PIPE = 109
    SEM_TIMEOUT = 121
    INVALID_NAME = 123
    BAD_PATHNAME = 161
    BAD_PIPE = 230
    PIPE_BUSY = 231
    PIPE_NOT_CONNECTED = 233
#}

class CommandPiper: #{
    CONNECT_TIMEOUT_MS = 500
    CONNECT_NUM_RETRIES = 5
    NO_DATA_STR = '{{1}}'
    STOP_CMD = '{{3}}'
    FORCE_STOP_CMD = '{{4}}'
    win_err = WinErr()
    
    def __init__(self, logger, pipe_name, buffer_size, init_event_name): #{
        self.pipe_name = pipe_name
        self.pipe_path = self._create_pipe_path(pipe_name)
        self.buffer_size = buffer_size
        
        self._logger = logger
        self._init_event_name = init_event_name
        self._cancel_token = True
        self._pipe = None
    #}
    
    def _create_pipe_path(self, pipe_name): #{
        return '\\\\.\\pipe\\' + pipe_name
    #}
    
    def _is_err(self, ex, err_codes): #{
        for err_code in err_codes: #{
            if ex.args[0] == err_code: #{
                return True
            #}
        #}
        return False
    #}
    
    def is_enabled(self): #{
        return self._cancelToken == False;
    #}
    
    def _stop(self, force=False): #{
        self._cancel_token = True
        
        if force and self._is_pipe_set: #{
            self._close_pipe()
        #}
    #}
    
    def start(self, sync=True): #{
        self._cancel_token = False       
        
        if sync and not self._send_sync_event(): #{
            self._logger.error('Failed to sync with server on command piper init for pipe: ' + self.pipe_name)
            self._cancel_token = True
            return
        #}
        
        if not self._open_pipe(): #{
            self._logger.error('Could not start command piper due to being unable to open pipe: ' + self.pipe_name)
            self._cancel_token = True
            return
        #}
        
        try: #{
            self._logger.info('Starting command piper for pipe: ' + self.pipe_name)
            should_reset_pipe = False
            
            while not self._cancel_token: #{
                if should_reset_pipe and not self._reset_pipe(): #{
                    self._logger.error('Failed to reset pipe "' + self.pipe_name +  '". Stopping piper process.')
                    break
                #}
                
                cmd_success, should_reset_pipe, unhandled_error = self._process_pipe_command()
                
                if unhandled_error: #{
                    break
                #}
            #}
            
            self._logger.info('Stopping command piper for pipe: ' + self.pipe_name)
        #}
        finally: #{
            self._stop()
        #}
    #}
    
    def _send_sync_event(self): #{
        event = None
        event_name = self._init_event_name
        
        try: #{
            self._logger.info('Sending sync event "' + event_name + '" to server for pipe: ' + self.pipe_name)
            
            event = win32event.OpenEvent(win32event.EVENT_MODIFY_STATE, False, event_name)
            win32event.SetEvent(event)
            
            self._logger.info('Sync event "' + event_name + '" sent to server for pipe: ' + self.pipe_name)
            return True
        #}
        except pywintypes.error as ex: #{
            self._logger.error('Failed to open sync event "' + event_name + '" for pipe: ' + self.pipe_name, exc_info=True)
            return False
        #}
        finally: #{
            if event: #{
                win32file.CloseHandle(event)
            #}
        #}
    #}
    
    def _is_pipe_set(self): #{
        return self._pipe is not None
    #}
    
    def _open_pipe(self, timeout_ms=None, num_retries=None): #{
        if self._pipe: #{
            self._logger.warning('Cannot open a pipe that is already open. Pipe name: ' + self.pipe_name)
            return True
        #}
        
        self._logger.info('Attempting to open pipe handle: ' + self.pipe_name)
        
        if self._wait_for_pipe_to_connect(timeout_ms, num_retries): #{
            self._pipe = self._get_pipe()
        #}
        
        pipe_open = self._is_pipe_set()
        
        if pipe_open: #{
            self._logger.info('Connected to pipe: ' + self.pipe_name)
            self._cancel_token = False
        #}
        else: #{
            self._logger.error('Unable to connect to pipe: ' + self.pipe_name)
            self._pipe = None
        #}
        
        return pipe_open
    #}
    
    def _wait_for_pipe_to_connect(self, timeout_ms=None, num_retries=None): #{
        if self._cancel_token:
            return False
        if timeout_ms == None:
            timeout_ms = self.CONNECT_TIMEOUT_MS
        if num_retries == None:
            num_retries = self.CONNECT_NUM_RETRIES
        
        try: #{
            win32pipe.WaitNamedPipe(self.pipe_path, timeout_ms)
            return True
        #}
        except pywintypes.error as ex: #{
            err_msg = 'Failed to find pipe "' + self.pipe_name + '" within expected time period.'
            r = self.win_err
            
            if num_retries > 0 and self._is_err(ex, [r.SEM_TIMEOUT, r.FILE_NOT_FOUND, r.BROKEN_PIPE, r.BAD_PIPE, r.PIPE_BUSY, r.PIPE_NOT_CONNECTED]): #{
                self._logger.warning(err_msg)
                
                if self._is_err(ex, [r.FILE_NOT_FOUND]): #{
                    time.sleep(timeout_ms / 1000)
                #}
                
                self._logger.info('Pipe "' + self.pipe_name + '" connection reattempts left: ' + str(num_retries))
                return self._wait_for_pipe_to_connect(timeout_ms, num_retries - 1)
            #}
            else: #{
                self._logger.error(err_msg, exc_info=True)
            #}
            
            return False
        #}
    #}
    
    def _get_pipe(self): #{
        try: #{
            pipe = win32file.CreateFile(
                self.pipe_path, # file name
                win32file.GENERIC_READ | win32file.GENERIC_WRITE, # desired access
                0, # share mode
                None, # attributes
                win32file.OPEN_EXISTING, # creation disposition
                win32file.FILE_ATTRIBUTE_NORMAL, # flags and attributed
                None # template file
            )
            
            win32pipe.SetNamedPipeHandleState(pipe, win32pipe.PIPE_READMODE_MESSAGE, None, None)            
            return pipe
        #}
        except pywintypes.error as ex: #{
            self._logger.error("Failed to get pipe: " + self.pipe_name, exc_info=True)
            return None
        #}
    #}
    
    def _close_pipe(self): #{
        if not self._is_pipe_set(): #{
            self._logger.warning('Cannot close an unassigned or already closed pipe. Pipe name: ' + self.pipe_name)
            return
        #}
        
        self._logger.info('Closing pipe handle: ' + self.pipe_name)
        win32file.CloseHandle(self._pipe)
        self._pipe = None
    #}
    
    def _reset_pipe(self, timeout_ms=None, num_retries=None): #{
        self._logger.info('Attempting to reconnect to pipe: ' + self.pipe_name)
        self._close_pipe()
        
        return self._open_pipe(timeout_ms, num_retries)
    #}
    
    def _process_pipe_command(self): #{
        cmd_success = False
        should_reset_pipe = False
        unhandled_error = False
        
        try: #{
            command = self._read_from_pipe()
            output, cmd_success = self._execute_command(command)
            self._write_to_pipe(output)
        #}
        except pywintypes.error as ex: #{
            r = self.win_err
            
            if self._is_err(ex, [r.INVALID_HANDLE, r.BROKEN_PIPE, r.BAD_PIPE, r.PIPE_NOT_CONNECTED]): #{
                self._logger.warning('Connection to pipe "' + self.pipe_name + '" no longer valid. Will attempt to reconnect.', exc_info=True)
                should_reset_pipe = True
            #}
            else: #{
                self._logger.error('An unexpected error occurred for pipe: ' + self.pipe_name, exc_info=True)
                unhandled_error = True
            #}
        #}
        
        return cmd_success, should_reset_pipe, unhandled_error
    #}
    
    def _write_to_pipe(self, output): #{
        if not self._is_pipe_set(): #{
            self._logger.warning('Attempted to write to an unset pipe: ' + self.pipe_name)
            return
        #}
        if not output: #{
            output = self.NO_DATA_STR
        #}
        
        pipe = self._pipe
        self._logger.debug('Writing data to pipe "' + self.pipe_name + '": ' + output)
        
        win32file.WriteFile(pipe, output.encode())
        win32file.FlushFileBuffers(pipe)
    #}
    
    def _read_from_pipe(self): #{
        if not self._is_pipe_set(): #{
            self._logger.warning('Attempted to read from an unset pipe: ' + self.pipe_name)
            return ''
        #}
        
        pipe = self._pipe
        output = ''
        nAvail = 1
        
        while not self._cancel_token and nAvail > 0: #{
            output += win32file.ReadFile(pipe, self.buffer_size)[1].decode()
            nAvail = win32pipe.PeekNamedPipe(pipe, 0)[1]
        #}
        
        self._logger.debug('Data read from pipe "' + self.pipe_name + '": '
            + (output if len(output) < 300 else output[:100] + '...\n'))
        
        return output
    #}
    
    def _execute_command(self, command): #{
        if not command: #{
            return self.NO_DATA_STR, False
        #}
        
        temp_stdout = sys.stdout
        output = ''
        success = False
        
        try: #{
            buffer = StringIO()
            sys.stdout = buffer
            
            if command == self.STOP_CMD: #{
                self._stop(False)
            #}
            elif command == self.FORCE_STOP_CMD: #{
                self._stop(True)
            #}
            else: #{
                exec(command, globals())
                output = buffer.getvalue() or ''
            #}
            
            success = True
        #}
        except BaseException as ex: #{
            self._logger.error('Failed to run command from pipe "' + self.pipe_name + '": ' + command, exc_info=True)
        #}
        finally: #{
            sys.stdout = temp_stdout
        #}
        
        return self.NO_DATA_STR if not output else output[:-1], success
    #}
#}

)";

        static const string _initPythonTemplate2 = R"(
class CmdPiperManager: #{
    MANAGER_PIPER_ID = '{{2}}'
    
    def __init__(self, logger, pipe_base_name, buffer_size): #{
        self._logger = logger
        self._pipe_base_name = pipe_base_name
        self._buffer_size = buffer_size
        
        self._thread_tracker = { }
        self._manager_piper = self._create_piper(self.MANAGER_PIPER_ID)
    #}
    
    def _create_pipe_name(self, pipe_id): #{
        return self._pipe_base_name + '-' + pipe_id
    #}
    
    def _create_piper(self, pipe_id, buffer_size_override = None): #{
        pipe_name = self._create_pipe_name(pipe_id)
        buffer_size = buffer_size_override or self._buffer_size
        
        return CommandPiper(self._logger, pipe_name, buffer_size, pipe_name)
    #}
    
    def start_manager_piper(self, wait_for_all=False): #{
        try: #{
            self._manager_piper.start()
        #}
        finally: #{
            if wait_for_all: #{
                self._wait_for_all_threads_end()
            #}
        #}
    #}
    
    def start_piper(self, piper_id, buffer_size_override = None): #{
        if self._thread_exists(piper_id): #{
            thread = self._get_thread(piper_id)
            
            if thread.is_alive(): #{
                return False
            #}
            
            self._untrack_thread(piper_id)
        #}
        
        piper = self._create_piper(piper_id, buffer_size_override)
        thread = self._create_thread(piper_id, piper)
        
        self._track_thread(piper_id, thread)
        thread.start()
        return True
    #}
    
    def _create_thread(self, piper_id, piper): #{
        return threading.Thread(target=self._piper_thread_action, args=[piper_id, piper])
    #}
    
    def _piper_thread_action(self, piper_id, piper): #{
        try: #{
            piper.start()
        #}
        finally: #{
            self._untrack_thread(piper_id)
        #}
    #}
    
    def _track_thread(self, piper_id, thread): #{
        self._thread_tracker[piper_id] = thread
    #}
    
    def _thread_exists(self, piper_id): #{
        return piper_id in self._thread_tracker
    #}
    
    def _get_thread(self, piper_id): #{
        return self._thread_tracker[piper_id] if self._thread_exists(piper_id) else None
    #}
    
    def _untrack_thread(self, piper_id): #{
        if self._thread_exists(piper_id): #{
            del self._thread_tracker[piper_id]
        #}
    #}
    
    def _wait_for_all_threads_end(self, timeout=None): #{
        for piper_id in self._thread_tracker.copy(): #{
            self._wait_for_thread_end(piper_id, timeout)
        #}
    #}
    
    def _wait_for_thread_end(self, piper_id, timeout=None): #{
        thread = self._get_thread(piper_id)
        if thread == None: #{
            self._logger.warning('No tracked thread associated with id: ' + piper_id)
            return
        #}

        thread.join(timeout)
        
        if thread.is_alive(): #{
            self._logger.warning('Thread "' + piper_id + '" has not terminated within timeout period of ' + str(timeout) + ' seconds.')            
        #}
    #}
#}


class Console(Popen): #{
    _encoding = 'utf-8'
    _cmd_template = """
import sys, os, locale

os.system("color {0}")
os.system("title {1}")
encoding = '{2}'

for line in sys.stdin:
    try:
        sys.stdout.buffer.write(line.encode(encoding))
        sys.stdout.flush()
    except BaseException as ex:
        print(ex)
"""
    
    def __init__(self, color=None, title=None): #{
        cmd = self._cmd_template.format(color or '', title or '', self._encoding)
        cmd = sys.executable, "-c", cmd
        
        os.environ['PYTHONIOENCODING'] = self._encoding
        super().__init__(cmd, stdin=PIPE, bufsize=1, text=True, creationflags=CREATE_NEW_CONSOLE, encoding=self._encoding)
    #}
    
    def is_alive(self): #{
        return self.poll() == None
    #}
    
    def write(self, msg): #{
        self.stdin.write(msg + '\n')
        self.stdin.flush()
    #}
#}

class ConsoleHandler(logging.Handler): #{
    def __init__(self, title=None): #{
        self.console = Console(title=title)
        logging.Handler.__init__(self=self)
    #}
    
    def emit(self, record): #{
        if not self.console.is_alive():
            return
        
        msg = self._apply_color(record.levelno, self.format(record))
        self.console.write(msg)
    #}
    
    def _apply_color(self, level, msg): #{
        if level >= logging.ERROR:
            return '\033[91m' + msg + '\033[0m'
        elif level >= logging.WARNING:
           return '\033[93m' + msg + '\033[0m'
        else:
            return msg
    #}
#}

class LoggerWriter: #{
    _formatter = logging.Formatter('[%(asctime)s][%(levelname)s] %(message)s')
    
    def __init__(self, logger, level): #{
        self.logger = logger
        self.level = level
    #}
    
    def write(self, message): #{
        if message and message.strip():
            self.logger.log(self.level, message.strip())
    #}
    
    def flush(self): #{
        pass
    #}

    @staticmethod
    def create_logger(name, logLevel, logPath, use_console_logger=0): #{
        logger = logging.getLogger(name)
        
        f_hndl = RotatingFileHandler(logPath, encoding='utf-8', maxBytes=10*1024*1024, backupCount=0)
        f_hndl.setFormatter(LoggerWriter._formatter)
        logger.addHandler(f_hndl)
        
        if use_console_logger != 0: #{
            c_hndl = ConsoleHandler('Testing')
            c_hndl.setFormatter(logging.Formatter('[%(asctime)s][%(levelname)s] %(message)s'))
            logger.addHandler(c_hndl)
        #}
        
        logger.setLevel(logLevel)
        return logger
    #}
#}

)";
        static const string _initPythonTemplateFull = _initPythonTemplate1 + _initPythonTemplate2;
        string initPython = WinApiHelper::replace(_initPythonTemplateFull, "{{1}}", PyNoData);
        initPython = WinApiHelper::replace(initPython, "{{2}}", _managerPipeId);
        initPython = WinApiHelper::replace(initPython, "{{3}}", StopCmd);
        initPython = WinApiHelper::replace(initPython, "{{4}}", ForceStopCmd);
        return initPython;
    }

    string getMainExecCode(const string& id) const {
        static const string _mainExecCode = R"(

import sys
sys.dont_write_bytecode = True
import logging
import {{1}} as ____

if __name__ == '__main__': #{
    script_name = sys.argv[0]
    use_log_console = int(sys.argv[3])
    log_level = int(sys.argv[4])
    log_path = sys.argv[5]
    
    ____.main_logger = ____.LoggerWriter.create_logger(
        script_name, log_level, log_path, use_log_console
    )
    sys.stdout = ____.LoggerWriter(____.main_logger, logging.INFO)
    sys.stderr = ____.LoggerWriter(____.main_logger, logging.ERROR)
    ____.main_logger.info('***** Initialization script started: ' + script_name)

    root_name = sys.argv[1]
    buffer_size = int(sys.argv[2])
    
    ____.___piper_manager = ____.CmdPiperManager(
        ____.main_logger, root_name, buffer_size
    )
    
    try: #{
        ____.___piper_manager.start_manager_piper(True)
    #}
    finally: #{
        ____.main_logger.info('Process ended for script: ' + script_name)
    #}
#}

)";

        string moduleName = idToModuleName(id);
        return WinApiHelper::replace(_mainExecCode, "{{1}}", moduleName);
    }
};
