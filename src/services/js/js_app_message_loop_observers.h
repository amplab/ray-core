// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_JS_JS_APP_MESSAGE_LOOP_OBSERVERS_H_
#define SERVICES_JS_JS_APP_MESSAGE_LOOP_OBSERVERS_H_

#include "base/message_loop/message_loop.h"
#include "mojo/message_pump/message_pump_mojo.h"
#include "v8/include/v8.h"

namespace js {

class JSAppMessageLoopObservers {
 public:
  explicit JSAppMessageLoopObservers(v8::Isolate* isolate);
  ~JSAppMessageLoopObservers();

 private:
  class TaskObserver : public base::MessageLoop::TaskObserver {
   public:
    explicit TaskObserver(JSAppMessageLoopObservers* loop_observers);
    void WillProcessTask(const base::PendingTask& pending_task) override;
    void DidProcessTask(const base::PendingTask& pending_task) override;
   private:
    JSAppMessageLoopObservers* loop_observers_;
  };

  class SignalObserver : public mojo::common::MessagePumpMojo::Observer {
   public:
    explicit SignalObserver(JSAppMessageLoopObservers* loop_observers);
    void WillSignalHandler() override;
    void DidSignalHandler() override;
  private:
    JSAppMessageLoopObservers* loop_observers_;
  };

  void RunMicrotasks();

  v8::Isolate* isolate_;
  TaskObserver task_observer_;
  SignalObserver signal_observer_;
};

}  // namespace js

#endif  // SERVICES_JS_JS_APP_MESSAGE_LOOP_OBSERVERS_H_
