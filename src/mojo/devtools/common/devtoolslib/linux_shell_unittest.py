# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for the linux shell abstraction."""

import imp
import os.path
import sys
import unittest
import tempfile
import shutil
import threading

try:
  imp.find_module("devtoolslib")
except ImportError:
  sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from devtoolslib.linux_shell import LinuxShell


class LinuxShellTest(unittest.TestCase):
  """Tests the Linux shell abstraction.

  These do not actually run the shell binary, it is substituted for shell utils
  like sleep or cat.
  """

  def test_run_and_get_output(self):
    """Verifies that run_and_get_output() correctly builds and passes the
    argument list to the binary.
    """
    shell = LinuxShell('echo')
    shell_args = ['--some-argument 42', 'unicornA', 'unicornB']
    return_code, output, did_time_out = shell.run_and_get_output(shell_args)

    self.assertEquals(0, return_code)
    self.assertEquals(' '.join(shell_args), output.strip())
    self.assertEquals(False, did_time_out)

  def test_run_and_get_output_timeout_met(self):
    """Verifies the returned values of run_and_get_output() when timeout is set
    but the binary exits before it is hit.
    """
    shell = LinuxShell('echo')
    shell_args = ['--some-argument 42', 'unicornA', 'unicornB']
    return_code, output, did_time_out = shell.run_and_get_output(shell_args,
                                                                 timeout=1)

    self.assertEquals(0, return_code)
    self.assertEquals(' '.join(shell_args), output.strip())
    self.assertEquals(False, did_time_out)

  def test_run_and_get_output_timeout_exceeded(self):
    """Verifies the returned values of run_and_get_output() when timeout is set
    and the binary does not exit before it is hit.
    """
    temp_dir = tempfile.mkdtemp()
    fifo_path = os.path.join(temp_dir, 'fifo')
    os.mkfifo(fifo_path)

    class Data:
      fifo = None

    # Any write to the fifo will block until it is open for reading by cat,
    # hence write on a background thread.
    def _write_to_fifo():
      Data.fifo = open(fifo_path, 'w')
      print >> Data.fifo, 'abc'
      Data.fifo.flush()
    write_thread = threading.Thread(target=_write_to_fifo)
    write_thread.start()

    # The call to cat should read what is written to the fifo ('abc') and then
    # stall forever, as we don't close the fifo after writing.
    shell = LinuxShell('cat')
    args = [fifo_path]
    _, output, did_time_out = shell.run_and_get_output(args, timeout=1)

    write_thread.join()
    if Data.fifo:
      Data.fifo.close()

    # Verify that the process did time out and that the output was correctly
    # produced before that.
    self.assertEquals(True, did_time_out)
    self.assertEquals('abc', output.strip())
    shutil.rmtree(temp_dir)


if __name__ == "__main__":
  unittest.main()
