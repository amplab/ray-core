// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEBUGGER_TRACE_COLLECTOR_H_
#define SERVICES_DEBUGGER_TRACE_COLLECTOR_H_

#include <vector>

#include "base/callback.h"
#include "mojo/data_pipe_utils/data_pipe_drainer.h"

namespace debugger {

class TraceCollector : public mojo::common::DataPipeDrainer::Client {
 public:
  typedef base::Callback<void(std::string)> TraceCallback;

  explicit TraceCollector(mojo::ScopedDataPipeConsumerHandle source);
  ~TraceCollector() override;

  void GetTrace(TraceCallback callback);

 private:
  void OnDataAvailable(const void* data, size_t num_bytes) override;
  void OnDataComplete() override;

  std::string GetTraceAsString();

  mojo::common::DataPipeDrainer drainer_;
  std::vector<char> trace_;
  bool is_complete_;
  TraceCallback callback_;
};

}  // namespace debugger

#endif  // SERVICES_DEBUGGER_TRACE_COLLECTOR_H_
