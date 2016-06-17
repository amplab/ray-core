// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gin/public/v8_platform.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/threading/thread.h"
#include "gin/per_isolate_data.h"

namespace gin {

namespace {

base::LazyInstance<V8Platform>::Leaky g_v8_platform = LAZY_INSTANCE_INITIALIZER;

}  // namespace

// static
V8Platform* V8Platform::Get() { return g_v8_platform.Pointer(); }

V8Platform::V8Platform() {}

V8Platform::~V8Platform() {}

void V8Platform::CallOnBackgroundThread(
    v8::Task* task,
    v8::Platform::ExpectedRuntime expected_runtime) {
  if (!background_thread_) {
    background_thread_.reset(new base::Thread("gin_background"));
    background_thread_->Start();
  }
  background_thread_->task_runner()->PostTask(
      FROM_HERE, base::Bind(&v8::Task::Run, base::Owned(task)));
}

void V8Platform::CallOnForegroundThread(v8::Isolate* isolate, v8::Task* task) {
  PerIsolateData::From(isolate)->task_runner()->PostTask(
      FROM_HERE, base::Bind(&v8::Task::Run, base::Owned(task)));
}

double V8Platform::MonotonicallyIncreasingTime() {
  return base::TimeTicks::Now().ToInternalValue() /
      static_cast<double>(base::Time::kMicrosecondsPerSecond);
}

}  // namespace gin
