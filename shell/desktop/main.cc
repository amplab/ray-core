// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <iostream>

#include "base/at_exit.h"
#include "base/base_switches.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/debug/profiler.h"
#include "base/files/file_util.h"
#include "base/message_loop/message_loop.h"
#include "base/synchronization/waitable_event.h"
#include "base/trace_event/trace_event.h"
#include "shell/command_line_util.h"
#include "shell/context.h"
#include "shell/init.h"
#include "shell/switches.h"
#include "shell/tracer.h"

namespace {

void Usage() {
  std::cerr << "Launch Mojo applications.\n";
  std::cerr
      << "Usage: mojo_shell"
      << " [--" << switches::kArgsFor << "=<mojo-app>]"
      << " [--" << switches::kContentHandlers << "=<handlers>]"
      << " [--" << switches::kCPUProfile << "]"
      << " [--" << switches::kDisableCache << "]"
      << " [--" << switches::kEnableMultiprocess << "]"
      << " [--" << switches::kOrigin << "=<url-lib-path>]"
      << " [--" << switches::kTraceStartup << "[=\"list,of,categories\"]]"
      << " [--" << switches::kTraceStartupDuration << "=<seconds>]"
      << " [--" << switches::kTraceStartupOutputName << "=<file_name>]"
      << " [--" << switches::kURLMappings << "=from1=to1,from2=to2]"
      << " [--" << switches::kWaitForDebugger << "]"
      << " <mojo-app> ...\n\n"
      << "A <mojo-app> is a Mojo URL or a Mojo URL and arguments within "
      << "quotes.\n"
      << "Example: mojo_shell \"mojo:js_standalone test.js\".\n"
      << "<url-lib-path> is searched for shared libraries named by mojo URLs.\n"
      << "The value of <handlers> is a comma separated list like:\n"
      << "text/html,mojo:html_viewer,"
      << "application/javascript,mojo:js_content_handler\n";
}

}  // namespace

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  base::CommandLine::Init(argc, argv);

  shell::Tracer tracer;
  shell::InitializeLogging();

  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kHelp) ||
      command_line.GetArgs().empty()) {
    Usage();
    return 0;
  }

  const std::set<std::string> all_switches = switches::GetAllSwitches();
  const base::CommandLine::SwitchMap switches = command_line.GetSwitches();
  for (const auto& s : switches) {
    if (all_switches.find(s.first) == all_switches.end()) {
      std::cerr << "Unknown switch: " << s.first << std::endl;
      Usage();
      return 1;
    }
  }

  bool trace_startup = command_line.HasSwitch(switches::kTraceStartup);
  if (trace_startup) {
    std::string output_name =
        command_line.GetSwitchValueASCII(switches::kTraceStartupOutputName);
    tracer.Start(
        command_line.GetSwitchValueASCII(switches::kTraceStartup),
        command_line.GetSwitchValueASCII(switches::kTraceStartupDuration),
        output_name.empty() ? "mojo_shell.trace" : output_name);
  }

  if (command_line.HasSwitch(switches::kCPUProfile)) {
#if !defined(NDEBUG) || !defined(ENABLE_PROFILING)
    LOG(ERROR) << "Profiling requires is_debug=false and "
               << "enable_profiling=true. Continuing without profiling.";
// StartProfiling() and StopProfiling() are a no-op.
#endif
    base::debug::StartProfiling("mojo_shell.pprof");
  }

  // We want the shell::Context to outlive the MessageLoop so that pipes are all
  // gracefully closed / error-out before we try to shut the Context down.
  shell::Context shell_context(&tracer);
  // There is no authentication service on desktop.
  shell_context.url_resolver()->AddURLMapping(
      GURL("mojo:authenticated_network_service"), GURL("mojo:network_service"));
  {
    base::MessageLoop message_loop;
    tracer.DidCreateMessageLoop();
    if (!shell_context.Init()) {
      Usage();
      return 1;
    }

    message_loop.PostTask(
        FROM_HERE, base::Bind(&shell::RunCommandLineApps, &shell_context));
    message_loop.Run();

    // Must be called before |message_loop| is destroyed.
    shell_context.Shutdown();
  }

  if (command_line.HasSwitch(switches::kCPUProfile))
    base::debug::StopProfiling();
  if (trace_startup)
    tracer.StopAndFlushToFile();
  return 0;
}
