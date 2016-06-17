// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/switches.h"

#include "base/base_switches.h"
#include "base/macros.h"

namespace switches {

// Specify configuration arguments for a Mojo application URL. For example:
// --args-for='mojo:wget http://www.google.com'
const char kArgsFor[] = "args-for";

// Comma separated list like:
// text/html,mojo:html_viewer,application/bravo,https://abarth.com/bravo
const char kContentHandlers[] = "content-handlers";

// Starts sampling based CPU profiling when the shell starts up. The profile is
// written to disk when the shell exits.
const char kCPUProfile[] = "cpu-profile";

// Force dynamically loaded apps / services to be loaded irrespective of cache
// instructions.
const char kDisableCache[] = "disable-cache";

// If set apps downloaded are not deleted.
const char kDontDeleteOnDownload[] = "dont-delete-on-download";

// Load apps in separate processes.
// TODO(vtl): Work in progress; doesn't work. Flip this to "disable" (or maybe
// change it to "single-process") when it works.
const char kEnableMultiprocess[] = "enable-multiprocess";

// In multiprocess mode, force these apps to be loaded in the main process.
// Comma-separate list of URLs. Example:
// --force-in-process=mojo:native_viewport_service,mojo:network_service
const char kForceInProcess[] = "force-in-process";

// Forces offline by default mode, even for local URLs.
const char kForceOfflineByDefault[] = "force-offline-by-default";

// Print the usage message and exit.
const char kHelp[] = "help";

// Specify origin to map to base url. See url_resolver.cc for details.
// Can be used multiple times.
const char kMapOrigin[] = "map-origin";

// Map mojo: URLs to a shared library of similar name at this origin. See
// url_resolver.cc for details.
const char kOrigin[] = "origin";

// Starts tracing when the shell starts up, saving a trace file on disk after 5
// seconds or when the shell exits.
const char kTraceStartup[] = "trace-startup";

// Sets the time in seconds until startup tracing ends. If omitted a default of
// 5 seconds is used.
const char kTraceStartupDuration[] = "trace-startup-duration";

// Sets the name of the output file for startup tracing. If omitted a default of
// 'mojo_shell.trace' is used.
const char kTraceStartupOutputName[] = "trace-startup-output-name";

// Specifies a set of mappings to apply when resolving urls. The value is a set
// of ',' separated mappings, where each mapping consists of a pair of urls
// giving the to/from url to map. For example, 'a=b,c=d' contains two mappings,
// the first maps 'a' to 'b' and the second 'c' to 'd'.
const char kURLMappings[] = "url-mappings";

// Switches valid for the main process (i.e., that the user may pass in).
const char* const kSwitchArray[] = {
    kArgsFor, kContentHandlers, kCPUProfile, kDisableCache,
    kDontDeleteOnDownload, kEnableMultiprocess, kForceInProcess,
    kForceOfflineByDefault, kHelp, kMapOrigin, kOrigin, kTraceStartup,
    kTraceStartupDuration, kTraceStartupOutputName, kURLMappings,
    // |base| switches we "support":
    kV, kWaitForDebugger};

const std::set<std::string> GetAllSwitches() {
  std::set<std::string> switch_set;

  for (size_t i = 0; i < arraysize(kSwitchArray); ++i)
    switch_set.insert(kSwitchArray[i]);
  return switch_set;
}

}  // namespace switches
