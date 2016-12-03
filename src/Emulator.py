# Emulator.py
# Copyright 2016 Steve Palmer

# from __future__ import absolute_import, division, print_function, unicode_literals

import sys
from threading import Thread
import signal

if sys.version_info.major == 2:
    import subprocess32 as subprocess
else:
    import subprocess

from robot.api import logger
from robot.utils import ConnectionCache
from robot.utils import asserts

# http://stackoverflow.com/questions/6893968/how-to-get-the-return-value-from-a-thread-in-python
class ThreadWithReturnAndLog(Thread):
    '''
    The Library reference states "...only override the __init__() and run() methods of this class."
    However, the trick below seems to work ...
    '''
    def __init__(self, group=None, target=None, name=None, args=(), kwargs={}):
        Thread.__init__(self, group, target, name, args, kwargs)
        self._return = None
    def run(self):
        if self._Thread__target is not None:
            self._return = self._Thread__target(*self._Thread__args, **self._Thread__kwargs)
    def join(self, timeout=None):
        super(ThreadWithReturnAndLog, self).join(timeout)
        return self._return

__metaclass__ = type

class Emulator:
    """A test library providing communication with an emulated machine.
    """
    ROBOT_LIBRARY_SCOPE = 'TEST_SUITE'

    def __init__(self,
                 executable="./emulator",
                 config_file="testrc.xml",
                 prompt=">",
                 log_level='INFO',
                 timeout=1.0):
        self._executable = executable
        self._config_file = config_file
        self._default_prompt = prompt
        self._log_level = log_level
        self._timeout = timeout
        self._cache = ConnectionCache()
        self._thread = None

    def _log(self, msg, log_level=None):
        msg = msg.strip()
        if (msg):
            logger.write(msg, log_level or self._log_level)

    def log_executable_name(self, log_level=None):
        self._log(self._executable, log_level)
    
    def log_configuration_file(self, log_level=None):
        self._log(self._config_file, log_level)

    def _execute(self, thread, log_level=None, timeout=None):
        connection = self._cache.get_connection() 
        connection.clear_log()
        thread.start()
        result = thread.join(timeout or self._timeout)
        self._log(connection.log, log_level)
        if thread.is_alive():
            asserts.fail("Timedout")
            connection.clear_log()
            connection.reset()
            self._log(connection.log, log_level)
        return result

    def open_connection(self, alias=None,
                        executable=None,
                        config_file=None,
                        prompt=None,
                        log_level=None,
                        timeout=None):
        connection = EmulatorConnection(executable or self._executable,
                                        config_file or self._config_file,
                                        prompt or self._default_prompt,
                                        timeout or self._timeout)
        self._log(connection.log, log_level)
        result = self._cache.register(connection, alias)
        self._log("Active Connection: %s" % repr(result))
        return result

    def switch_connection(self, index_or_alias, log_level=None):
        old_index = self._cache.current_index
        self._cache.switch(index_or_alias)
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
        self._conn = self._cache.close_all()

    def close_connection(self, log_level=None, timeout=None):
        """Closes the current connection"""
        self._execute(ThreadWithReturnAndLog(target=self._cache.get_connection().close_connection,
                                             name='close_connection'),
                      log_level, timeout or self._timeout * 3)
        self.current_index = None
        self._log("Active Connection: %s" % repr(self._cache.current_index), log_level)

    def set_prompt(self, prompt=None):
        self._cache.get_connection().prompt = prompt or self._default_prompt

    def set_log_level(self, log_level=None):
        self._cache.get_connection().log_level = log_level or self._log_level

    def write_bare(self, text, log_level=None, timeout=None):
        self._execute(ThreadWithReturnAndLog(target=self._cache.get_connection().write_bare,
                                             args=(text,),
                                             name='write_bare'),
                      log_level, timeout)

    def write(self, text, log_level=None, timeout=None):
        self._execute(ThreadWithReturnAndLog(target=self._cache.get_connection().write,
                                             args=(text,),
                                             name='write'),
                      log_level, timeout)

    def read(self, log_level=None, timeout=None):
        result = self._execute(ThreadWithReturnAndLog(target=self._cache.get_connection().read,
                                                     name='read'),
                               log_level, timeout)
        return result

    def read_line(self, log_level=None, timeout=None):
        result = self._execute(ThreadWithReturnAndLog(target=self._cache.get_connection().read_line,
                                                     name='read_line'),
                               log_level, timeout)
        return result

    def read_until(self, expect, log_level=None, timeout=None):
        result = self._execute(ThreadWithReturnAndLog(target=self._cache.get_connection().read_until,
                                                     args=(expect,),
                                                     name='read_until'),
                               log_level, timeout)
        return result

    def read_until_prompt(self, log_level=None, timeout=None):
        result = self._execute(ThreadWithReturnAndLog(target=self._cache.get_connection().read_until_prompt,
                                                     name='read_until_prompt'),
                               log_level, timeout)
        return result

    def reset(self, log_level=None, timeout=None):
        self._execute(ThreadWithReturnAndLog(target=self._cache.get_connection().reset,
                                             name='reset'),
                      log_level, timeout)

class EmulatorConnection:
    """Handles the state associated with a specific emulator run
    """

    def __init__(self, executable, config_file, prompt, timeout):
        self._prompt = prompt
        self._timeout = timeout
        command = [executable,'-f',config_file]
        self._log = "Exec:%s\n" % (" ".join(command))
        self._popen = subprocess.Popen(command, bufsize=0, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        self._popen.poll()
        self._pid = self._popen.pid
        self._returncode = self._popen.returncode
        asserts.assert_none(self._returncode)

    @property
    def prompt(self): return self._prompt
    
    @prompt.setter
    def prompt(self, prompt): self._prompt = prompt

    def clear_log(self):
        self._log=""

    @property
    def log(self): return self._log

    def _read_log(self, text):
        self._log += "Rx:%s\n" % repr(text)

    def _write_log(self, text):
        self._log += "Tx:%s\n" % repr(text)

    def close(self):
        if self._popen:
            asserts.assert_none(self._returncode)
            self._popen.terminate()
            try:
                self._popen.wait(self._timeout)
                self._returncode = self._popen.returncode
            except subprocess.TimeoutExpired:
                pass
            if self._returncode is None:
                self._popen.kill()
                try:
                    self._popen.wait(self._timeout)
                    self._returncode = self._popen.returncode
                except subprocess.TimeoutExpired:
                    pass
            self._log += "Return Code:%s\n" % repr(self._returncode)

    def close_connection(self):
        if self._popen:
            self._popen.stdin.close()
            result = self.read()
            self._popen.poll()
            self._returncode = self._popen.returncode
            self._log += "Return Code:%s\n" % repr(self._returncode)
            self._popen = None
        if self._popen:
            self.close()
        self._popen = None
        return result

    def write_bare(self, text):
        self._write_log(text)
        if (self._popen):
            self._popen.stdin.write(text)

    def write(self, text):
        text += '\n'
        self.write_bare(text)
        self.read_until(text)

    def read(self):
        result = self._popen.stdout.read() if self._popen else ''
        self._read_log(result)
        return result

    def read_line(self):
        result = self._popen.stdout.readline() if self._popen else ''
        self._read_log(result)
        return result

    def _read_until(self, expected):
        result = ''
        while self._popen and not result.endswith(expected):
            result += self._popen.stdout.read(1)
        return result

    def read_until(self, expected):
        result = self._read_until(expected)
        self._read_log(result)
        result = result[:-len(expected)]
        return result

    def read_until_prompt(self):
        result = self._read_until(self._prompt)
        self._read_log(result)
        result = result[:-len(self._prompt)]
        return result

    def reset(self):
        if self._popen:
            self._popen.send_signal(signal.SIGUSR1)
            # self._popen.stdin.write(" ")  # to unblock the processor
            self._log += "Connection Reset\n"
