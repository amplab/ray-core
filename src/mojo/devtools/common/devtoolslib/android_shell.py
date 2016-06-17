# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import atexit
import hashlib
import logging
import os
import os.path
import random
import re
import subprocess
import sys
import tempfile
import threading
import time
import uuid

from devtoolslib import http_server
from devtoolslib.shell import Shell
from devtoolslib.utils import overrides


# Tags used by mojo shell Java logging.
_LOGCAT_JAVA_TAGS = [
    'AndroidHandler',
    'MojoFileHelper',
    'MojoShellApplication',
    'ShellService',
]

_MOJO_SHELL_PACKAGE_NAME = 'org.chromium.mojo.shell'

# Used to parse the output of `adb devices`.
_ADB_DEVICES_HEADER = 'List of devices attached'

# Fixed port on which Flutter observatory is run.
_FLUTTER_OBSERVATORY_PORT = 8181


_logger = logging.getLogger()


def _exit_if_needed(process):
  """Exits |process| if it is still alive."""
  if process.poll() is None:
    process.kill()


def _find_available_port(netstat_output, max_attempts=10000):
  opened = [int(x.strip().split()[3].split(':')[1])
            for x in netstat_output if x.startswith(' tcp')]
  for _ in xrange(max_attempts):
    port = random.randint(4096, 16384)
    if port not in opened:
      return port
  else:
    raise Exception('Failed to identify an available port.')


def _find_available_host_port():
  netstat_output = subprocess.check_output(['netstat'])
  return _find_available_port(netstat_output)


def parse_adb_devices_output(adb_devices_output):
  """Parses the output of the `adb devices` command, returning a dictionary
  mapping device id to the status of the device, as printed by `adb devices`.
  """
  # Split into lines skipping empty ones.
  lines = [line.strip() for line in adb_devices_output.split('\n')
           if line.strip()]

  if _ADB_DEVICES_HEADER not in lines:
    return None

  # The header can be preceeded by output informing of adb server being spawned,
  # but all non-empty lines after the header describe connected devices.
  device_specs = lines[lines.index(_ADB_DEVICES_HEADER) + 1:]
  split_specs = [spec.split() for spec in device_specs]
  return {split_spec[0]: split_spec[1] for split_spec in split_specs
          if len(split_spec) == 2}


class AndroidShell(Shell):
  """Wrapper around Mojo shell running on an Android device.

  Args:
    adb_path: Path to adb, optional if adb is in PATH.
    target_device: Device to run on, if multiple devices are connected.
    logcat_tags: Comma-separated list of additional logcat tags to use.
  """

  def __init__(self, adb_path="adb", target_device=None, logcat_tags=None,
               verbose=False):
    self.adb_path = adb_path
    self.target_device = target_device
    self.stop_shell_registered = False
    self.additional_logcat_tags = logcat_tags
    self.verbose_stdout = sys.stdout if verbose else open(os.devnull, 'w')
    self.verbose_stderr = sys.stderr if verbose else self.verbose_stdout

  def _adb_command(self, args):
    """Forms an adb command from the given arguments, prepending the adb path
    and adding a target device specifier, if needed.
    """
    adb_command = [self.adb_path]
    if self.target_device:
      adb_command.extend(['-s', self.target_device])
    adb_command.extend(args)
    return adb_command

  def _read_fifo(self, fifo_path, pipe, on_fifo_closed, max_attempts=5):
    """Reads |fifo_path| on the device and write the contents to |pipe|.

    Calls |on_fifo_closed| when the fifo is closed. This method will try to find
    the path up to |max_attempts|, waiting 1 second between each attempt. If it
    cannot find |fifo_path|, a exception will be raised.
    """
    fifo_command = self._adb_command(
        ['shell', 'run-as', _MOJO_SHELL_PACKAGE_NAME, 'ls', fifo_path])

    def _run():
      def _wait_for_fifo():
        for _ in xrange(max_attempts):
          output = subprocess.check_output(fifo_command).strip()
          if output == fifo_path:
            return
          time.sleep(1)
        if on_fifo_closed:
          on_fifo_closed()
        raise Exception('Timed out waiting for shell the create the fifo file.')
      _wait_for_fifo()
      stdout_cat = subprocess.Popen(
          self._adb_command(['shell', 'run-as', _MOJO_SHELL_PACKAGE_NAME,
                             'cat', fifo_path]), stdout=pipe)
      atexit.register(_exit_if_needed, stdout_cat)
      stdout_cat.wait()
      if on_fifo_closed:
        on_fifo_closed()

    thread = threading.Thread(target=_run, name='StdoutRedirector')
    thread.start()

  def _find_available_device_port(self):
    netstat_output = subprocess.check_output(
        self._adb_command(['shell', 'netstat']))
    return _find_available_port(netstat_output)

  def _forward_device_port_to_host(self, device_port, host_port):
    """Maps the device port to the host port. If |device_port| is 0, a random
    available port is chosen.

    Returns:
      The device port.
    """
    assert host_port

    if device_port == 0:
      # TODO(ppi): Should we have a retry loop to handle the unlikely races?
      device_port = self._find_available_device_port()
    subprocess.check_call(self._adb_command([
        'reverse', 'tcp:%d' % device_port, 'tcp:%d' % host_port]))

    def _unmap_port():
      unmap_command = self._adb_command([
          'reverse', '--remove', 'tcp:%d' % device_port])
      subprocess.Popen(unmap_command)
    atexit.register(_unmap_port)
    return device_port

  def _forward_host_port_to_device(self, host_port, device_port):
    """Maps the host port to the device port. If |host_port| is 0, a random
    available port is chosen.

    Returns:
      The host port.
    """
    assert device_port

    if host_port == 0:
      # TODO(ppi): Should we have a retry loop to handle the unlikely races?
      host_port = _find_available_host_port()
    subprocess.check_call(self._adb_command([
        'forward', 'tcp:%d' % host_port, 'tcp:%d' % device_port]))

    def _unmap_port():
      unmap_command = self._adb_command([
          'forward', '--remove', 'tcp:%d' % device_port])
      subprocess.Popen(unmap_command)
    atexit.register(_unmap_port)
    return host_port

  def _is_shell_package_installed(self):
    # Adb should print one line if the package is installed and return empty
    # string otherwise.
    return len(subprocess.check_output(self._adb_command([
        'shell', 'pm', 'list', 'packages', _MOJO_SHELL_PACKAGE_NAME]))) > 0

  def _get_api_level(self):
    """Returns the API level of Android running on the device."""
    output = subprocess.check_output(self._adb_command([
        'shell', 'getprop', 'ro.build.version.sdk']))
    return int(output)

  @staticmethod
  def get_tmp_dir_path():
    """Returns a path to a cache directory owned by the shell where temporary
    files can be stored.
    """
    return '/data/data/%s/cache/tmp/' % _MOJO_SHELL_PACKAGE_NAME

  def pull_file(self, device_path, destination_path, remove_original=False):
    """Copies or moves the specified file on the device to the host."""
    subprocess.check_call(self._adb_command([
        'pull', device_path, destination_path]))
    if remove_original:
      subprocess.check_call(self._adb_command([
          'shell', 'rm', device_path]))

  def check_device(self):
    """Verifies if the device configuration allows adb to run.

    If a target device was indicated in the constructor, it checks that the
    device is available. Otherwise, it checks that there is exactly one
    available device.

    Returns:
      A tuple of (result, msg). |result| is True iff if the device is correctly
      configured and False otherwise. |msg| is the reason for failure if
      |result| is False and None otherwise.
    """
    adb_devices_output = subprocess.check_output(
        self._adb_command(['devices']))
    devices = parse_adb_devices_output(adb_devices_output)

    if not devices:
      return False, 'No devices connected.'

    if self.target_device:
      if (self.target_device in devices and
          devices[self.target_device] == 'device'):
        return True, None
      else:
        return False, ('Cannot connect to the selected device, status: ' +
                       devices[self.target_device])

    if len(devices) > 1:
      return False, ('More than one device connected and target device not '
                     'specified.')

    if not devices.itervalues().next() == 'device':
      return False, 'Connected device is not available.'

    return True, None

  def install_apk(self, shell_apk_path):
    """Installs the apk on the device.

    This method computes checksum of the APK and skips the installation if the
    fingerprint matches the one saved on the device upon the previous
    installation.

    Args:
      shell_apk_path: Path to the shell Android binary.
    """
    device_sha1_path = '/sdcard/%s/%s.sha1' % (_MOJO_SHELL_PACKAGE_NAME,
                                               'MojoShell')
    apk_sha1 = hashlib.sha1(open(shell_apk_path, 'rb').read()).hexdigest()
    device_apk_sha1 = subprocess.check_output(self._adb_command([
        'shell', 'cat', device_sha1_path]))
    do_install = (apk_sha1 != device_apk_sha1 or
                  not self._is_shell_package_installed())

    if do_install:
      install_command = ['install']
      install_command += ['-r']  # Allow to overwrite an existing installation.
      install_command += ['-i', _MOJO_SHELL_PACKAGE_NAME]
      if self._get_api_level() >= 23:  # Check if running Lollipop or later.
        # Grant all permissions listed in manifest. This flag is available only
        # in Lollipop or later.
        install_command += ['-g']
      install_command += [shell_apk_path]
      subprocess.check_call(self._adb_command(install_command),
          stdout=self.verbose_stdout, stderr=self.verbose_stderr)

      # Update the stamp on the device.
      with tempfile.NamedTemporaryFile() as fp:
        fp.write(apk_sha1)
        fp.flush()
        subprocess.check_call(self._adb_command(['push', fp.name,
                                                device_sha1_path]),
                              stdout=self.verbose_stdout,
                              stderr=self.verbose_stderr)
    else:
      # To ensure predictable state after running install_apk(), we need to stop
      # the shell here, as this is what "adb install" implicitly does.
      self.stop_shell()

  def start_shell(self,
                 arguments,
                 stdout=None,
                 on_application_stop=None):
    """Starts the mojo shell, passing it the given arguments.

    Args:
      arguments: List of arguments for the shell.
      stdout: Valid argument for subprocess.Popen() or None.
    """
    if not self.stop_shell_registered:
      atexit.register(self.stop_shell)
      self.stop_shell_registered = True

    STDOUT_PIPE = '/data/data/%s/stdout.fifo' % _MOJO_SHELL_PACKAGE_NAME

    cmd = self._adb_command(['shell', 'am', 'start',
                            '-S',
                            '-a', 'android.intent.action.VIEW',
                            '-n', '%s/.MojoShellActivity' %
                            _MOJO_SHELL_PACKAGE_NAME])

    parameters = []
    if stdout or on_application_stop:
      # Remove any leftover fifo file after the previous run.
      subprocess.check_call(self._adb_command(
          ['shell', 'run-as', _MOJO_SHELL_PACKAGE_NAME,
           'rm', '-f', STDOUT_PIPE]))

      parameters.append('--fifo-path=%s' % STDOUT_PIPE)
      self._read_fifo(STDOUT_PIPE, stdout, on_application_stop)
    parameters.extend(arguments)

    if parameters:
      device_filename = (
          '/sdcard/%s/args_%s' % (_MOJO_SHELL_PACKAGE_NAME, str(uuid.uuid4())))
      with tempfile.NamedTemporaryFile(delete=False) as temp:
        try:
          for parameter in parameters:
            temp.write(parameter)
            temp.write('\n')
          temp.close()
          subprocess.check_call(self._adb_command(
              ['push', temp.name, device_filename]),
              stdout=self.verbose_stdout, stderr=self.verbose_stderr)
        finally:
          os.remove(temp.name)

      cmd += ['--es', 'argsFile', device_filename]

    subprocess.check_call(cmd, stdout=self.verbose_stdout,
                          stderr=self.verbose_stderr)

  def stop_shell(self):
    """Stops the mojo shell."""
    subprocess.check_call(self._adb_command(['shell',
                                            'am',
                                            'force-stop',
                                            _MOJO_SHELL_PACKAGE_NAME]))

  def clean_logs(self):
    """Cleans the logs on the device."""
    subprocess.check_call(self._adb_command(['logcat', '-c']))

  def show_logs(self):
    """Displays the log for the mojo shell.

    Returns:
      The process responsible for reading the logs.
    """
    tags = _LOGCAT_JAVA_TAGS
    if self.additional_logcat_tags is not None:
      tags.extend(self.additional_logcat_tags.split(","))
    logcat = subprocess.Popen(
        self._adb_command(['logcat', '-s', ' '.join(tags)]),
        stdout=sys.stdout)
    atexit.register(_exit_if_needed, logcat)
    return logcat

  def forward_observatory_ports(self):
    """Forwards the ports used by the dart observatories to the host machine.
    """
    logcat = subprocess.Popen(self._adb_command(['logcat']),
                              stdout=subprocess.PIPE)
    atexit.register(_exit_if_needed, logcat)

    def _forward_observatories_as_needed():
      while True:
        line = logcat.stdout.readline()
        if not line:
          break
        match = re.search(r'Observatory listening on http://127.0.0.1:(\d+)',
                          line)
        if match:
          device_port = int(match.group(1))
          host_port = self._forward_host_port_to_device(0, device_port)
          print ('Dart observatory available at the host at http://127.0.0.1:%d'
                 % host_port)

    logcat_watch_thread = threading.Thread(
        target=_forward_observatories_as_needed)
    logcat_watch_thread.daemon = True
    logcat_watch_thread.start()

  def forward_flutter_observatory_port(self):
    """Forwards the fixed port on which Flutter observatory is run."""
    self._forward_host_port_to_device(_FLUTTER_OBSERVATORY_PORT,
                                      _FLUTTER_OBSERVATORY_PORT)

  @overrides(Shell)
  def serve_local_directories(self, mappings, port, reuse_servers=False):
    assert mappings
    if reuse_servers:
      assert port, 'Cannot reuse the server when |port| is 0.'
      server_address = ('127.0.0.1', port)
    else:
      server_address = http_server.start_http_server(mappings, port)

    return 'http://127.0.0.1:%d/' % self._forward_device_port_to_host(
        port, server_address[1])

  @overrides(Shell)
  def forward_host_port_to_shell(self, host_port):
    self._forward_host_port_to_device(host_port, host_port)

  @overrides(Shell)
  def run(self, arguments):
    self.clean_logs()
    self.forward_observatory_ports()
    self.forward_flutter_observatory_port()

    p = self.show_logs()
    self.start_shell(arguments, sys.stdout, p.terminate)
    p.wait()
    return None

  @overrides(Shell)
  def run_and_get_output(self, arguments, timeout=None):
    class Results:
      """Workaround for Python scoping rules that prevent assigning to variables
      from the outer scope.
      """
      output = None

    def do_run():
      (r, w) = os.pipe()
      with os.fdopen(r, 'r') as rf:
        with os.fdopen(w, 'w') as wf:
          self.start_shell(arguments, wf, wf.close)
          Results.output = rf.read()

    run_thread = threading.Thread(target=do_run)
    run_thread.start()
    run_thread.join(timeout)

    if run_thread.is_alive():
      self.stop_shell()
      return None, Results.output, True
    return None, Results.output, False
