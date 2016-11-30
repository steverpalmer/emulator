# Emulator.py
# Copyright 2016 Steve Palmer

# from __future__ import absolute_import, division, print_function, unicode_literals

import sys
import signal

if sys.version_info.major == 2:
    import subprocess32 as subprocess
else:
    import subprocess

from robot.api import logger
from robot.utils import ConnectionCache
from robot.utils import asserts

__metaclass__ = type

class Emulator:
    """A test library providing communication with an emulated machine.
    """
    ROBOT_LIBRARY_SCOPE = 'TEST_SUITE'

    def __init__(self, prompt=">", default_log_level='INFO'):
        self._default_prompt = prompt
        self._default_log_level = default_log_level
        self._cache = ConnectionCache()

    def _log(self, msg, level=None):
        msg = msg.strip()
        if (msg):
            logger.write(msg, level or self._default_log_level)

    def open_connection(self, alias=None, prompt=None, log_level=None):
        connection = EmulatorConnection(prompt or self._default_prompt,
                                        log_level or self._default_log_level)
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

    def close_connection(self, log_level=None):
        """Closes the current connection"""
        self._cache.get_connection().close_connection()
        self.current_index = None
        self._log("Active Connection: %s" % repr(self._cache.current_index), log_level)

    def set_prompt(self, prompt=None):
        self._cache.get_connection().set_prompt(prompt or self._default_prompt)

    def set_log_level(self, level=None):
        self._cache.get_connection().set_log_level(level or self._default_log_level)

    def write_bare(self, text, log_level=None):
        self._cache.get_connection().write_bare(text, log_level)

    def write(self, text, log_level=None):
        self._cache.get_connection().write(text, log_level)

    def read(self, log_level=None):
        result = self._cache.get_connection().read(log_level)
        return result

    def read_line(self, log_level=None):
        result = self._cache.get_connection().read_line(log_level)
        return result

    def read_until(self, expect, log_level=None):
        result = self._cache.get_connection().read_until(expect, log_level)
        return result

    def read_until_prompt(self, log_level=None):
        result = self._cache.get_connection().read_until_prompt(log_level)
        return result

    def reset(self, log_level=None):
        result = self._cache.get_connection().reset(log_level)
        return result

class EmulatorConnection:
    """Handles the state associated with a specific emulator run
    """

    def __init__(self, prompt, log_level):
        self._prompt = prompt
        self._log_level = log_level
        self._popen = subprocess.Popen(['./emulator'],
                                       bufsize=0,
                                       stdin=subprocess.PIPE,
                                       stdout=subprocess.PIPE)
        self._popen.poll()
        self._pid = self._popen.pid
        self._returncode = self._popen.returncode
        asserts.assert_none(self._returncode)

    def set_prompt(self, prompt):
        self._prompt = prompt

    def set_log_level(self, log_level):
        self._log_level = log_level

    def _log(self, msg, level=None):
        msg = msg.strip()
        if (msg):
            logger.write(msg, level or self._log_level)

    def _read_log(self, text, log_level=None):
        self._log("Rx:" + repr(text), log_level)

    def _write_log(self, text, log_level=None):
        self._log("Tx:" + repr(text), log_level)

    def close(self):
        if self._popen:
            asserts.assert_none(self._returncode)
            self._popen.terminate()
            try:
                self._popen.wait(1)
                self._returncode = self._popen.returncode
            except TimeoutExpired:
                pass
            if self._returncode is None:
                self._popen.kill()
                try:
                    self._popen.wait(1)
                    self._returncode = self._popen.returncode
                except TimeoutExpired:
                    pass
            self._log("Return Code:" + repr(self._returncode))

    def close_connection(self):
        if self._popen:
            self._popen.stdin.close()
            # self._popen.wait ???
            result = self.read()
            self._popen.poll()
            self._returncode = self._popen.returncode
            self._log("Return Code:" + repr(self._returncode))
            self._popen = None
        if self._popen:
            self.close()
        self._popen = None
        return result

    def write_bare(self, text, log_level=None):
        self._write_log(text)
        if (self._popen):
            self._popen.stdin.write(text)

    def write(self, text, log_level=None):
        text += '\n'
        self.write_bare(text, log_level)
        self.read_until(text, log_level)

    def read(self, log_level=None):
        result = self._popen.stdout.read() if self._popen else ''
        self._read_log(result, log_level)
        return result

    def read_line(self, log_level=None):
        result = self._popen.stdout.readline() if self._popen else ''
        self._read_log(result, log_level)
        return result

    def _read_until(self, expected):
        result = ''
        while self._popen and not result.endswith(expected):
            result += self._popen.stdout.read(1)
        return result

    def read_until(self, expected, log_level=None):
        result = self._read_until(expected)
        self._read_log(result, log_level)
        result = result[:-len(expected)]
        return result

    def read_until_prompt(self, log_level=None):
        result = self._read_until(self._prompt)
        self._read_log(result, log_level)
        result = result[:-len(self._prompt)]
        return result

    def reset(self, log_level=None):
        if self._popen:
            self._popen.send_signal(signal.SIGUSR1)
            # self._popen.stdin.write(" ")  # to unblock the processor
            self._log("Connection Reset", log_level)
