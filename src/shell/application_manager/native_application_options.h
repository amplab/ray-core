// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_APPLICATION_MANAGER_NATIVE_APPLICATION_OPTIONS_H_
#define SHELL_APPLICATION_MANAGER_NATIVE_APPLICATION_OPTIONS_H_

namespace shell {

// Options relevant to running native applications. (Note that some of these may
// be set at different stages, as more is learned about the application.)
// TODO(vtl): Maybe these some of these should be tristate (unknown/true/false)
// instead of of booleans.
struct NativeApplicationOptions {
  bool new_process_per_connection = false;
  bool force_in_process = false;
  bool require_32_bit = false;
  bool allow_new_privs = false;
};

}  // namespace shell

#endif  // SHELL_APPLICATION_MANAGER_NATIVE_APPLICATION_OPTIONS_H_
