# Emulator.py
# Copyright 2016 Steve Palmer

# from __future__ import absolute_import, division, print_function, unicode_literals

import sys

if sys.version_info.major == 2:
    import subprocess32 as subprocess
else:
    import subprocess

from robot.api import logger
from robot.utils import ConnectionCache

__metaclass__ = type

class Emulator:
    """A test library providing communication with an emulated machine.
    """
    ROBOT_LIBRARY_SCOPE = 'TEST_SUITE'

    def __init__(self):
        self._cache = ConnectionCache()

    def open_connection(self, alias=None):
        logger.info("Opening Connection to emulator")
        connection = EmulatorConnection()
        return self._cache.register(connection, alias)

    def switch_connection(self, index_or_alias):
        old_index = self._cache.current_index
        self._cache.switch(index_or_alias)
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

    def close_connection(self):
        """Closes the current connection"""
        self._cache.get_connection().close_connection()
        self.current_index = None

    def write_bare(self, text):
        self._cache.get_connection().write_bare(text)

    def write(self, text):
        self._cache.get_connection().write(text)

    def read(self):
        result = self._cache.get_connection().read()
        return result

    def read_line(self):
        result = self._cache.get_connection().read_line()
        logger.info("read_line: " + repr(result))
        return result

    def read_until(self, expect):
        result = self._cache.get_connection().read_until(expect)
        return result
    
class EmulatorConnection:
    """Handles the state associated with a specific emulator run
    """

    def __init__(self):
        self._popen = subprocess.Popen(['./emulator'],
                                       bufsize=0,
                                       stdin=subprocess.PIPE,
                                       stdout=subprocess.PIPE)

    def close(self):
        if self._popen:
            self._popen.terminate()
            try:
                self._popen.wait(1)
            except TimeoutExpited:
                self._popen.kill()
            
    def close_connection(self):
        self.close()
        result = self.read()
        self._popen = None
        return result

    def write_bare(self, text):
        if (self._popen):
            self._popen.stdin.write(text)

    def write(self, text):
        self.write_base(text + '\n')
        return self.read()

    def read(self):
        result = self._popen.stdout.read() if self._popen else ""
        return result

    def read_line(self):
        result = self._popen.stdout.readline() if self._popen else ""
        return result
    
    def read_until(self, expected):
        result = ""
        while self._popen and not result.endswith(expected):
            result += self.read_line()
        return result
