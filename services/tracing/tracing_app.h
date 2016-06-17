// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_TRACING_TRACING_APP_H_
#define SERVICES_TRACING_TRACING_APP_H_

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "mojo/common/binding_set.h"
#include "mojo/common/interface_ptr_set.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/tracing/interfaces/trace_provider_registry.mojom.h"
#include "mojo/services/tracing/interfaces/tracing.mojom.h"
#include "services/tracing/trace_data_sink.h"
#include "services/tracing/trace_recorder_impl.h"

namespace tracing {

class TracingApp : public mojo::ApplicationImplBase,
                   public TraceCollector,
                   public TraceProviderRegistry {
 public:
  TracingApp();
  ~TracingApp() override;

 private:
  // mojo::ApplicationImplBase override.
  bool OnAcceptConnection(
      mojo::ServiceProviderImpl* service_provider_impl) override;

  // TraceCollector implementation.
  void Start(mojo::ScopedDataPipeProducerHandle stream,
             const mojo::String& categories) override;
  void StopAndFlush() override;

  // TraceProviderRegistry implementation.
  void RegisterTraceProvider(
      mojo::InterfaceHandle<TraceProvider> trace_provider) override;

  void AllDataCollected();

  scoped_ptr<TraceDataSink> sink_;
  ScopedVector<TraceRecorderImpl> recorder_impls_;
  mojo::InterfacePtrSet<TraceProvider> provider_ptrs_;
  mojo::Binding<TraceCollector> collector_binding_;
  mojo::BindingSet<TraceProviderRegistry> provider_registry_bindings_;
  bool tracing_active_;
  mojo::String tracing_categories_;

  DISALLOW_COPY_AND_ASSIGN(TracingApp);
};

}  // namespace tracing

#endif  // SERVICES_TRACING_TRACING_APP_H_
