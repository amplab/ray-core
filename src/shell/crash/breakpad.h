// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file was forked from components/crash/app/breakpad_linux.h in chromium.

// Public interface for enabling Breakpad on Linux systems.

#ifndef SHELL_CRASH_BREAKPAD_H_
#define SHELL_CRASH_BREAKPAD_H_

#include "base/files/file_path.h"

namespace breakpad {

// Turns on the crash reporter.
void InitCrashReporter(const base::FilePath& dumps_path);

// Checks if crash reporting is enabled.
bool IsCrashReporterEnabled();

}  // namespace breakpad

#endif  // SHELL_CRASH_BREAKPAD_H_
