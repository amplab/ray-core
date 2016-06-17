// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPS_BENCHMARK_TRACE_COORDINATOR_CLIENT_H_
#define APPS_BENCHMARK_TRACE_COORDINATOR_CLIENT_H_

#include <memory>
#include <string>

#include "base/memory/scoped_ptr.h"
#include "mojo/data_pipe_utils/data_pipe_drainer.h"
#include "mojo/services/tracing/interfaces/tracing.mojom.h"

// Connects to trace collector in tracing.mojo to get traces and returns the
// results as a single string to the receiver.
class TraceCollectorClient : public mojo::common::DataPipeDrainer::Client {
 public:
  class Receiver {
   public:
    // |trace_data| will be a JSON list of the collected trace events.
    virtual void OnTraceCollected(std::string trace_data) = 0;

   protected:
    virtual ~Receiver() {}
  };

  TraceCollectorClient(Receiver* receiver,
                       tracing::TraceCollectorPtr collector);
  ~TraceCollectorClient() override;

  void Start(const std::string& categories);
  void Stop();

 private:
  // mojo::common:DataPipeDrainer:
  void OnDataAvailable(const void* data, size_t num_bytes) override;
  void OnDataComplete() override;

  Receiver* receiver_;
  tracing::TraceCollectorPtr collector_;
  scoped_ptr<mojo::common::DataPipeDrainer> drainer_;
  std::string trace_data_;
  bool currently_tracing_;

  DISALLOW_COPY_AND_ASSIGN(TraceCollectorClient);
};

#endif  // APPS_BENCHMARK_TRACE_COORDINATOR_CLIENT_H_
