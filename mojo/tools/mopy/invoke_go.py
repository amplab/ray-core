# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""The function InvokeGo() is used to invoke the Go tool after setting up
the environment and current working directory."""

import os
import subprocess
import sys

NDK_PLATFORM = 'android-16'
NDK_TOOLCHAIN = 'arm-linux-androideabi-4.9'

def InvokeGo(go_tool, go_options, work_dir=None, src_root=None,
             out_root=None, cgo_cflags=None, cgo_ldflags=None,
             target_os='linux'):
  """ Invokes the go tool after setting up the environment.
  go_tool     The path to the 'go' binary. Must be either an absolute path
              or a path relative to the current working directory.

  go_options  A list of string arguments that will be passed to 'go'

  work_dir    Optional specification of a directory to temporarily switch into
              before invoking the go tool. If set it must be a path relative to
              the current working directory or an absolute path.

  src_root    Optional specification of the Mojo src root. If set it must be
              a path relative to the current working directory or an absolute
              path. This will be used to add additional elements to
              the GOPATH environment variable.

  out_root    Optional specification of the Mojo out dir. If set it must be
              a path relative to the current working directory or an absolute
              path. This will be used to add additional elements to
              the GOPATH environment variable.

  cgo_cflags  Optional value to set for the CGO_CFLAGS environment variable.

  cgo_ldflags Optional value to set for the CGO_LDFLAGS environment variable.

  target_os
              Optional value allows you to choose between 'linux' (default),
              'android' and 'mac'.
  """
  assert(target_os in ('linux', 'android', 'mac'))
  go_tool = os.path.abspath(go_tool)
  env = os.environ.copy()
  env['GOROOT'] = os.path.dirname(os.path.dirname(go_tool))

  go_path = ''
  if src_root is not None:
    # The src directory specified may be relative.
    src_root = os.path.abspath(src_root)
    # GOPATH must be absolute, and point to one directory up from |src_Root|
    go_path = os.path.abspath(os.path.join(src_root, '..'))
    # GOPATH also includes any third_party/go libraries that have been imported
    go_path += ':' +  os.path.join(src_root, 'third_party', 'go')

  if out_root is not None:
    go_path += ':' +  os.path.abspath(os.path.join(out_root, 'gen', 'go'))

  if 'MOJO_GOPATH' in os.environ:
      go_path += ':' + os.environ['MOJO_GOPATH']

  if len(go_path) > 0 :
    env['GOPATH'] = go_path

  if cgo_cflags is not None:
    env['CGO_CFLAGS'] = cgo_cflags

  if cgo_ldflags is not None:
    env['CGO_LDFLAGS'] = cgo_ldflags

  if target_os == 'android':
    env['CGO_ENABLED'] = '1'
    env['GOOS'] = 'android'
    env['GOARCH'] = 'arm'
    env['GOARM'] = '7'
    # The Android go tool prebuilt binary has a default path to the compiler,
    # which with high probability points to an invalid path, so we override the
    # CC env var that will be used by the go tool.
    if 'CC' not in env:
      if src_root is None:
        raise Exception('src_root must be set if is_android is true and '
                        '"CC" is not in env.')
      # Note that src_root was made absolute above.
      ndk_path = os.path.join(src_root, 'third_party', 'android_tools', 'ndk')
      if sys.platform.startswith('linux'):
        arch = 'linux-x86_64'
      elif sys.platform == 'darwin':
        arch = 'darwin-x86_64'
      else:
        raise Exception('unsupported platform: ' + sys.platform)
      ndk_cc = os.path.join(ndk_path, 'toolchains', NDK_TOOLCHAIN,
          'prebuilt', arch, 'bin', 'arm-linux-androideabi-gcc')
      sysroot = os.path.join(ndk_path, 'platforms', NDK_PLATFORM, 'arch-arm')
      env['CGO_CFLAGS'] += ' --sysroot %s' % sysroot
      env['CGO_LDFLAGS'] += ' --sysroot %s' % sysroot
      env['CC'] = '%s --sysroot %s' % (ndk_cc, sysroot)
  elif target_os == 'mac':
    env['CGO_ENABLED'] = '1'
    env['GOOS'] = 'darwin'
    env['GOARCH'] = 'amd64'

  save_cwd = os.getcwd()
  if work_dir is not None:
    os.chdir(work_dir)
  result = subprocess.call([go_tool] + go_options, env=env)
  if work_dir is not None:
    os.chdir(save_cwd)
  return result
