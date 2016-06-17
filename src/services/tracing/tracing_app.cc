// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/tracing/tracing_app.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/public/cpp/system/wait.h"

namespace tracing {

TracingApp::TracingApp() : collector_binding_(this), tracing_active_(false) {}

TracingApp::~TracingApp() {}

bool TracingApp::OnAcceptConnection(
    mojo::ServiceProviderImpl* service_provider_impl) {
  service_provider_impl->AddService<TraceCollector>(
      [this](const mojo::ConnectionContext& connection_context,
             mojo::InterfaceRequest<TraceCollector> trace_collector_request) {
        if (collector_binding_.is_bound()) {
          LOG(ERROR) << "Another application is already connected to tracing.";
          return;
        }

        collector_binding_.Bind(trace_collector_request.Pass());
      });
  service_provider_impl->AddService<TraceProviderRegistry>(
      [this](const mojo::ConnectionContext& connection_context,
             mojo::InterfaceRequest<TraceProviderRegistry>
                 trace_provider_registry_request) {
        provider_registry_bindings_.AddBinding(
            this, trace_provider_registry_request.Pass());
      });
  return true;
}

void TracingApp::Start(mojo::ScopedDataPipeProducerHandle stream,
                       const mojo::String& categories) {
  tracing_categories_ = categories;
  sink_.reset(new TraceDataSink(stream.Pass()));
  provider_ptrs_.ForAllPtrs([categories, this](TraceProvider* controller) {
    TraceRecorderPtr ptr;
    recorder_impls_.push_back(
        new TraceRecorderImpl(GetProxy(&ptr), sink_.get()));
    controller->StartTracing(categories, ptr.Pass());
  });
  tracing_active_ = true;
}

void TracingApp::StopAndFlush() {
  // Remove any collectors that closed their message pipes before we called
  // StopTracing(). See https://github.com/domokit/mojo/issues/225.
  for (int i = recorder_impls_.size() - 1; i >= 0; --i) {
    if (!recorder_impls_[i]->TraceRecorderHandle().is_valid()) {
      recorder_impls_.erase(recorder_impls_.begin() + i);
    }
  }

  tracing_active_ = false;
  provider_ptrs_.ForAllPtrs(
      [](TraceProvider* controller) { controller->StopTracing(); });

  // Sending the StopTracing message to registered controllers will request that
  // they send trace data back via the collector interface and, when they are
  // done, close the collector pipe. We don't know how long they will take. We
  // want to read all data that any collector might send until all collectors or
  // closed or an (arbitrary) deadline has passed. Since the bindings don't
  // support this directly we do our own MojoWaitMany over the handles and read
  // individual messages until all are closed or our absolute deadline has
  // elapsed.
  static const MojoDeadline kTimeToWaitMicros = 5 * 1000000;
  MojoTimeTicks end = MojoGetTimeTicksNow() + kTimeToWaitMicros;

  while (!recorder_impls_.empty()) {
    MojoTimeTicks now = MojoGetTimeTicksNow();
    if (now >= end)  // Timed out?
      break;

    MojoDeadline mojo_deadline = end - now;
    std::vector<mojo::Handle> handles;
    std::vector<MojoHandleSignals> signals;
    for (const auto& it : recorder_impls_) {
      handles.push_back(it->TraceRecorderHandle());
      signals.push_back(MOJO_HANDLE_SIGNAL_READABLE |
                        MOJO_HANDLE_SIGNAL_PEER_CLOSED);
    }
    std::vector<MojoHandleSignalsState> signals_states(signals.size());
    const mojo::WaitManyResult wait_many_result =
        mojo::WaitMany(handles, signals, mojo_deadline, &signals_states);
    if (wait_many_result.result == MOJO_RESULT_DEADLINE_EXCEEDED) {
      // Timed out waiting, nothing more to read.
      LOG(WARNING) << "Timed out waiting for trace flush";
      break;
    }
    if (wait_many_result.IsIndexValid()) {
      // Iterate backwards so we can remove closed pipes from |recorder_impls_|
      // without invalidating subsequent offsets.
      for (size_t i = signals_states.size(); i != 0; --i) {
        size_t index = i - 1;
        MojoHandleSignals satisfied = signals_states[index].satisfied_signals;
        // To avoid dropping data, don't close unless there's no
        // readable signal.
        if (satisfied & MOJO_HANDLE_SIGNAL_READABLE)
          recorder_impls_[index]->TryRead();
        else if (satisfied & MOJO_HANDLE_SIGNAL_PEER_CLOSED)
          recorder_impls_.erase(recorder_impls_.begin() + index);
      }
      // Something happened so push back the timeout deadline.
      end = MojoGetTimeTicksNow() + kTimeToWaitMicros;
    }
  }
  AllDataCollected();
}

void TracingApp::RegisterTraceProvider(
    mojo::InterfaceHandle<TraceProvider> trace_provider) {
  auto provider_ptr = TraceProviderPtr::Create(trace_provider.Pass());
  if (tracing_active_) {
    TraceRecorderPtr recorder_ptr;
    recorder_impls_.push_back(
        new TraceRecorderImpl(GetProxy(&recorder_ptr), sink_.get()));
    provider_ptr->StartTracing(tracing_categories_, recorder_ptr.Pass());
  }
  provider_ptrs_.AddInterfacePtr(provider_ptr.Pass());
}

void TracingApp::AllDataCollected() {
  recorder_impls_.clear();
  sink_.reset();
}

}  // namespace tracing
