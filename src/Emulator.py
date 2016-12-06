# Emulator.py
# Copyright 2016 Steve Palmer

import logging.config
logging.config.fileConfig(r'logging.conf')
LOG = logging.getLogger() if __name__ == '__main__' else logging.getLogger(__name__)


from sys import version_info
from os.path import exists
from threading import Timer
from signal import SIGUSR1

if version_info.major == 2:
    import subprocess32 as subprocess
else:
    import subprocess

from robot.api import logger
from robot.utils import ConnectionCache
from robot.utils import asserts

__metaclass__ = type

class EmulatorTimeoutException(Exception):
    pass

class Emulator:
    ''''
    A test library providing communication with an emulated machine.
    '''
    ROBOT_LIBRARY_SCOPE = 'TEST_SUITE'

    def __init__(self,
                 executable="./emulator",
                 config_file="testrc.xml",
                 prompt=">",
                 log_level='INFO'):
        LOG.debug("Emulator.__init__")
        self._executable = executable
        self._config_file = config_file
        self._default_prompt = prompt
        self._log_level = log_level
        self._cache = ConnectionCache()
        self._execute_connection = None

    def _log(self, msg, log_level=None):
        LOG.debug("Emulator._log")
        msg = msg.strip()
        if (msg):
            logger.write(msg, log_level or self._log_level)

    def log_executable_name(self, log_level=None):
        LOG.debug("Emulator.log_executable_name")
        self._log(self._executable, log_level)
    
    def log_configuration_file(self, log_level=None):
        LOG.debug("Emulator.log_configuration_file")
        self._log(self._config_file, log_level)

    def _timed_out(self, msg="Timed Out"):
        LOG.debug("Emulator._timed_out")
        if self._execute_connection is not None:
            # self._execute_connection.reset()
            self._execute_connection.close_connection()

    def _execute(self, alias, function, timeout, *args):
        LOG.debug("Emulator._execute")
        result = None
        asserts.assert_not_none(function, "Unknown Function")
        if timeout is None or isinstance(timeout, int) or isinstance(timeout, float):
            pass
        else:
            timeout = float(timeout)
        if callable(function):
            self._execute_connection = None
        else:
            self._execute_connection = self._cache.get_connection(alias)
            asserts.assert_not_none(self._execute_connection, "Connection Error")
            function = getattr(self._execute_connection, function, None)
            asserts.assert_not_none(function, "Unknown Function")
            asserts.assert_true(callable(function), "Function Not Callable")
        timer = Timer(timeout, self._timed_out) if timeout else None
        if timer:
            timer.start()
        result = function(*args)
        if timer:
            timer.cancel()
        self._execute_connection = None
        return result

    def open_connection(self,
                        alias=None,
                        executable=None,
                        config_file=None,
                        prompt=None,
                        log_level=None,
                        timeout=5.0):
        LOG.debug("Emulator.open_connection")
        executable = executable or self._executable
        config_file = config_file or self._config_file
        asserts.assert_true(exists(executable), "Executable does not exist")
        asserts.assert_true(exists(config_file), "Configuration File does not exist")
        connection = self._execute(None, EmulatorConnection, timeout,
                                   executable, config_file, prompt or self._default_prompt, log_level or self._log_level)
        result = self._cache.register(connection, alias)
        self._log("Active Connection: %s" % repr(result))
        return result

    def switch_connection(self, alias, log_level=None):
        LOG.debug("Emulator.switch_connection")
        old_index = self._cache.current_index
        self._cache.switch(alias)
        self._log("Active Connection: %s" % repr(self._cache.current_index), log_level)
        return old_index

    def close_all_connections(self):
        """Closes all open connections and empties the connection cache.

        If multiple connections are opened, this keyword should be used in
        a test or suite teardown to make sure that all connections are closed.
        It is not an error is some of the connections have already been closed
        by `Close Connection`.

        After this keyword, new indexes returned by `Open Connection`
        keyword are reset to 1.
        """
        LOG.debug("Emulator.close_all_connections")
        self._conn = self._cache.close_all()

    def close_connection(self, alias=None, log_level=None, timeout=5.0):
        """Closes the current connection"""
        LOG.debug("Emulator.close_connection")
        self._execute(alias, 'close_connection', timeout, log_level)
        self._cache.current_index = None
        self._log("Active Connection: %s" % repr(self._cache.current_index), log_level)

    def set_prompt(self, alias=None, prompt=None):
        LOG.debug("Emulator.set_prompt")
        self._cache.get_connection(alias).prompt = prompt or self._default_prompt

    def set_log_level(self, alias=None, log_level=None):
        LOG.debug("Emulator.set_log_level")
        self._cache.get_connection(alias).log_level = log_level or self._log_level

    def get_emulator_id(self, alias=None):
        LOG.debug("Emulator.get_emulator_id")
        return self._cache.get_connection(alias).get_id()
    
    def is_emulator_running(self, alias=None):
        LOG.debug("Emulator.is_emulator_running")
        return self._cache.get_connection(alias).is_running()

    def emulator_should_be_running(self, alias=None, error_message="Emulator is not running."):
        LOG.debug("Emulator.emulator_should_be_running")
        asserts.assert_true(self._cache.get_connection(alias).is_running(), error_message)
    
    def emulator_should_be_stopped(self, alias=None, error_message="Emulator is running."):
        LOG.debug("Emulator.emulator_should_be_stopped")
        try:
            asserts.assert_false(self._cache.get_connection(alias).is_running(), error_message)
        except:
            pass
    
    def write_bare(self, text, alias=None, log_level=None, timeout=1.0):
        LOG.debug("Emulator.write_bare")
        self._execute(alias, 'write_bare', timeout, text, log_level)

    def write(self, text, alias=None, log_level=None, timeout=1.0):
        LOG.debug("Emulator.write")
        self._execute(alias, 'write', timeout, text, log_level)

    def read(self, alias=None, log_level=None, timeout=None):
        LOG.debug("Emulator.read")
        result = self._execute(alias, 'read', timeout, log_level)
        return result

    def read_line(self, alias=None, log_level=None, timeout=None):
        LOG.debug("Emulator.read_line")
        result = self._execute(alias, 'read_line', timeout, log_level)
        return result

    def read_until(self, expect, alias=None, log_level=None, timeout=None):
        LOG.debug("Emulator.read_until")
        result = self._execute(alias, 'read_until', timeout, expect, log_level)
        return result

    def read_until_prompt(self, alias=None, log_level=None, timeout=None):
        LOG.debug("Emulator.read_until_prompt")
        result = self._execute(alias, 'read_until_prompt', timeout, log_level)
        return result

    def read_result(self, alias=None):
        LOG.debug("Emulator.read_result")
        result = self._cache.get_connection(alias).read_result
        return result

    def reset(self, alias=None, log_level=None, timeout=5.0):
        LOG.debug("Emulator.reset")
        self._execute(alias, 'reset', timeout, log_level)

class EmulatorConnection:
    """Handles the state associated with a specific emulator run
    """

    def __init__(self, executable, config_file, prompt, log_level):
        LOG.debug("EmulatorConnection.__init__")
        self._prompt = prompt
        self._log_level = log_level
        command = [executable,'-f',config_file]
        self._log("Exec:" + (" ".join(command)), log_level)
        self._popen = subprocess.Popen(command, bufsize=0, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        self._popen.poll()
        self._pid = self._popen.pid
        self._returncode = self._popen.returncode
        asserts.assert_none(self._returncode)
        self._read_result = ""

    @property
    def prompt(self): return self._prompt
    
    @prompt.setter
    def prompt(self, prompt): self._prompt = prompt

    @property
    def read_result(self): return self._read_result

    def _log(self, msg, log_level):
        LOG.debug("EmulatorConnection._log")
        msg = msg.strip()
        if (msg):
            logger.write(msg, log_level or self._log_level)

    def _read_log(self, text, log_level):
        LOG.debug("EmulatorConnection._read_log")
        self._log("Rx:" + text, log_level)

    def _write_log(self, text, log_level):
        LOG.debug("EmulatorConnection._write_log")
        self._log("Tx:" + text, log_level)

    def close(self, log_level=None):
        LOG.debug("EmulatorConnection.close")
        if self._popen:
            asserts.assert_none(self._returncode)
            self._popen.terminate()
            try:
                self._popen.wait(1.0)
                self._returncode = self._popen.returncode
            except subprocess.TimeoutExpired:
                pass
            if self._returncode is None:
                self._popen.kill()
                try:
                    self._popen.wait(1.0)
                    self._returncode = self._popen.returncode
                except subprocess.TimeoutExpired:
                    pass
            self._log("Return Code:" + repr(self._returncode), log_level)

    def close_connection(self, log_level=None):
        LOG.debug("EmulatorConnection.close_connection")
        result = ""
        if self._popen:
            self._popen.stdin.close()
            result = self.read(log_level)
            self._popen.poll()
            self._returncode = self._popen.returncode
            self._log("Return Code:" + repr(self._returncode), log_level)
            self._popen = None
        if self._popen:
            self.close()
        self._popen = None
        return result

    def get_id(self):
        LOG.debug("EmulatorConnection.get_id")
        return self._pid

    def is_running(self):
        LOG.debug("EmulatorConnection.is_running")
        return self._returncode is None

    def write_bare(self, text, log_level=None):
        LOG.debug("EmulatorConnection.write_bare")
        self._write_log(text, log_level)
        if (self._popen):
            self._popen.stdin.write(text)
        self._read_result = ""

    def write(self, text, log_level=None):
        LOG.debug("EmulatorConnection.write")
        text += '\n'
        self.write_bare(text, log_level)
        self.read_until(text, log_level)

    def read(self, log_level=None):
        LOG.debug("EmulatorConnection.read")
        result = self._popen.stdout.read() if self._popen else ''
        self._read_log(result, log_level)
        self._read_result = result
        return result

    def read_line(self, log_level=None):
        LOG.debug("EmulatorConnection.read_line")
        result = self._popen.stdout.readline() if self._popen else ''
        self._read_log(result, log_level)
        self._read_result = result
        return result

    def _read_until(self, expected):
        LOG.debug("EmulatorConnection._read_until")
        result = ''
        while True:
            if self._popen is None: break
            next_char = self._popen.stdout.read(1)
            if next_char=='': break
            result += next_char
            if result.endswith(expected): break
        LOG.debug("EmulatorConnection._read_until => %s", repr(result))
        return result

    def read_until(self, expected, log_level=None):
        LOG.debug("EmulatorConnection.read_until")
        result = self._read_until(expected)
        self._read_log(result, log_level)
        if result.endswith(expected):
            result = result[:-len(expected)]
        else:
            asserts.fail("Did not read expected response:%s" % result)
        self._read_result = result
        return result

    def read_until_prompt(self, log_level=None):
        LOG.debug("EmulatorConnection.read_until_prompt")
        result = self._read_until(self._prompt)
        self._read_log(result, log_level)
        if result.endswith(self._prompt):
            result = result[:-len(self._prompt)]
        else:
            asserts.fail("Did not read prompt:%s" % result)
        self._read_result = result
        return result

    def reset(self, log_level=None):
        LOG.debug("EmulatorConnection.reset")
        if self._popen:
            self._popen.send_signal(SIGUSR1)
            # self._popen.stdin.write(" ")  # to unblock the processor
            self._log("Connection Reset", log_level)
        self._read_result = ""

__all__ = ['Emulator']
