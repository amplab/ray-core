# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import subprocess
import threading
import tempfile

from devtoolslib import http_server
from devtoolslib.shell import Shell
from devtoolslib.utils import overrides


class LinuxShell(Shell):
  """Wrapper around Mojo shell running on Linux.

  Args:
    executable_path: path to the shell binary
    command_prefix: optional list of arguments to prepend to the shell command,
        allowing e.g. to run the shell under debugger.
  """

  def __init__(self, executable_path, command_prefix=None):
    self.executable_path = executable_path
    self.command_prefix = command_prefix if command_prefix else []

  @overrides(Shell)
  def serve_local_directories(self, mappings, port, reuse_servers=False):
    if reuse_servers:
      assert port, 'Cannot reuse the server when |port| is 0.'
      server_address = ('127.0.0.1', port)
    else:
      server_address = http_server.start_http_server(mappings, port)

    return 'http://%s:%d/' % server_address

  @overrides(Shell)
  def forward_host_port_to_shell(self, host_port):
    pass

  @overrides(Shell)
  def run(self, arguments):
    command = self.command_prefix + [self.executable_path] + arguments
    return subprocess.call(command, stderr=subprocess.STDOUT)

  @overrides(Shell)
  def run_and_get_output(self, arguments, timeout=None):
    command = self.command_prefix + [self.executable_path] + arguments
    output_file = tempfile.TemporaryFile()
    p = subprocess.Popen(command, stdout=output_file, stderr=output_file)

    wait_thread = threading.Thread(target=p.wait)
    wait_thread.start()
    wait_thread.join(timeout)

    did_time_out = False
    if p.poll() is None:
      did_time_out = True
      p.terminate()
      p.poll()
    output_file.seek(0)
    output = output_file.read()
    return p.returncode, output, did_time_out
