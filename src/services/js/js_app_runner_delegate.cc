// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/js/js_app_runner_delegate.h"

#include "base/path_service.h"
#include "gin/modules/console.h"
#include "gin/modules/timer.h"
#include "services/js/modules/clock/monotonic_clock.h"
#include "services/js/modules/gl/module.h"
#include "services/js/system/core.h"
#include "services/js/system/handle.h"
#include "services/js/system/support.h"
#include "services/js/system/threading.h"

namespace js {

namespace {

std::vector<base::FilePath> GetModuleSearchPaths() {
  std::vector<base::FilePath> search_paths(2);
  PathService::Get(base::DIR_SOURCE_ROOT, &search_paths[0]);
  PathService::Get(base::DIR_EXE, &search_paths[1]);
  search_paths[1] = search_paths[1].AppendASCII("gen");
  return search_paths;
}

// Shorten the AddBuiltinModule statements a little.
template<typename T>
void AddBuiltin(gin::ModuleRunnerDelegate* delegate) {
  delegate->AddBuiltinModule(T::kModuleName, T::GetModule);
}

} // namespace

JSAppRunnerDelegate::JSAppRunnerDelegate()
    : ModuleRunnerDelegate(GetModuleSearchPaths()) {
  AddBuiltin<gin::Console>(this);
  AddBuiltinModule(gin::TimerModule::kName, gin::TimerModule::GetModule);
  AddBuiltin<mojo::js::Core>(this);
  AddBuiltin<mojo::js::Support>(this);
  AddBuiltin<mojo::js::Threading>(this);
  AddBuiltin<gl::GL>(this);
  AddBuiltin<MonotonicClock>(this);
}

JSAppRunnerDelegate::~JSAppRunnerDelegate() {
}

void JSAppRunnerDelegate::UnhandledException(gin::ShellRunner* runner,
                                             gin::TryCatch& try_catch) {
  gin::ModuleRunnerDelegate::UnhandledException(runner, try_catch);
  LOG(ERROR) << try_catch.GetStackTrace();
}

} // namespace js

