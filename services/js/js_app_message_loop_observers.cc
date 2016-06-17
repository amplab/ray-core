// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/js/js_app_message_loop_observers.h"

namespace js {

JSAppMessageLoopObservers::JSAppMessageLoopObservers(v8::Isolate* isolate)
  : isolate_(isolate),
    task_observer_(this),
    signal_observer_(this) {
  base::MessageLoop::current()->AddTaskObserver(&task_observer_);
  mojo::common::MessagePumpMojo::current()->AddObserver(&signal_observer_);
}

JSAppMessageLoopObservers::~JSAppMessageLoopObservers() {
  base::MessageLoop::current()->RemoveTaskObserver(&task_observer_);
  mojo::common::MessagePumpMojo::current()->RemoveObserver(&signal_observer_);
}

JSAppMessageLoopObservers::TaskObserver::TaskObserver(
    JSAppMessageLoopObservers* loop_observers)
    : loop_observers_(loop_observers) {
}

void JSAppMessageLoopObservers::TaskObserver::WillProcessTask(
    const base::PendingTask& pending_task) {
}

void JSAppMessageLoopObservers::TaskObserver::DidProcessTask(
    const base::PendingTask& pending_task) {
  loop_observers_->RunMicrotasks();
}

JSAppMessageLoopObservers::SignalObserver::SignalObserver(
    JSAppMessageLoopObservers* loop_observers)
    : loop_observers_(loop_observers) {
}

void JSAppMessageLoopObservers::SignalObserver::WillSignalHandler() {
}

void JSAppMessageLoopObservers::SignalObserver::DidSignalHandler() {
  loop_observers_->RunMicrotasks();
}

void JSAppMessageLoopObservers::RunMicrotasks() {
  v8::Isolate::Scope scope(isolate_);
  isolate_->RunMicrotasks();
}

}  // namespace js
