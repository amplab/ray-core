// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/benchmark/trace_collector_client.h"

TraceCollectorClient::TraceCollectorClient(Receiver* receiver,
                                           tracing::TraceCollectorPtr collector)
    : receiver_(receiver),
      collector_(collector.Pass()),
      currently_tracing_(false) {}

TraceCollectorClient::~TraceCollectorClient() {}

void TraceCollectorClient::Start(const std::string& categories) {
  DCHECK(!currently_tracing_);
  currently_tracing_ = true;
  mojo::DataPipe data_pipe;
  collector_->Start(data_pipe.producer_handle.Pass(), categories);
  drainer_.reset(new mojo::common::DataPipeDrainer(
      this, data_pipe.consumer_handle.Pass()));
  trace_data_.clear();
  trace_data_ += "[";
}

void TraceCollectorClient::Stop() {
  DCHECK(currently_tracing_);
  currently_tracing_ = false;
  collector_->StopAndFlush();
}

void TraceCollectorClient::OnDataAvailable(const void* data, size_t num_bytes) {
  const char* chars = static_cast<const char*>(data);
  trace_data_.append(chars, num_bytes);
}

void TraceCollectorClient::OnDataComplete() {
  drainer_.reset();
  collector_.reset();
  trace_data_ += "]";
  receiver_->OnTraceCollected(trace_data_);
}
