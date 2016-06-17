// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DART_DART_TRACING_H_
#define SERVICES_DART_DART_TRACING_H_

#include "base/trace_event/trace_event.h"
#include "mojo/common/tracing_impl.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/services/tracing/interfaces/tracing.mojom.h"

namespace dart {

class DartTimelineController {
 public:
  static void Enable(const mojo::String& categories);
  static void EnableAll();
  static void Disable();
};

class DartTraceProvider : public tracing::TraceProvider {
 public:
  DartTraceProvider();
  ~DartTraceProvider() override;

  void Bind(mojo::InterfaceRequest<tracing::TraceProvider> request);

 private:
  // tracing::TraceProvider implementation:
  void StartTracing(
      const mojo::String& categories,
      mojo::InterfaceHandle<tracing::TraceRecorder> recorder) override;
  void StopTracing() override;

  void SplitAndRecord(char* data, size_t length);

  mojo::Binding<tracing::TraceProvider> binding_;
  tracing::TraceRecorderPtr recorder_;

  DISALLOW_COPY_AND_ASSIGN(DartTraceProvider);
};

class DartTracingImpl {
 public:
  DartTracingImpl();
  ~DartTracingImpl();

  // This connects to the tracing service and registers ourselves to provide
  // tracing data on demand.
  void Initialize(mojo::Shell* shell);

 private:
  DartTraceProvider provider_impl_;

  DISALLOW_COPY_AND_ASSIGN(DartTracingImpl);
};

}  // namespace dart

#endif  // SERVICES_DART_DART_TRACING_H_
