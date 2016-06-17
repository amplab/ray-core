// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "apps/benchmark/event.h"
#include "apps/benchmark/measurements.h"
#include "apps/benchmark/run_args.h"
#include "apps/benchmark/trace_collector_client.h"
#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "mojo/environment/scoped_chromium_init.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/bindings/interface_handle.h"
#include "mojo/public/interfaces/application/service_provider.mojom.h"
#include "mojo/services/tracing/interfaces/tracing.mojom.h"

namespace benchmark {
namespace {

class BenchmarkApp : public mojo::ApplicationImplBase,
                     public TraceCollectorClient::Receiver {
 public:
  BenchmarkApp() {}
  ~BenchmarkApp() override {}

  // mojo:ApplicationImplBase:
  void OnInitialize() override {
    // Parse command-line arguments.
    if (!GetRunArgs(args(), &args_)) {
      LOG(ERROR) << "Failed to parse the input arguments.";
      mojo::TerminateApplication(MOJO_RESULT_INVALID_ARGUMENT);
      return;
    }

    // Don't compute the categories string if all categories should be traced.
    std::string categories_str;
    if (args_.write_output_file) {
      categories_str = "*";
    } else {
      categories_str = ComputeCategoriesStr();
    }

    // Connect to trace collector, which will fetch the trace events produced by
    // the app being benchmarked.
    tracing::TraceCollectorPtr trace_collector;
    mojo::ConnectToService(shell(), "mojo:tracing", GetProxy(&trace_collector));
    trace_collector_client_.reset(
        new TraceCollectorClient(this, trace_collector.Pass()));
    trace_collector_client_->Start(categories_str);

    // Start tracing the application with 1 sec of delay.
    base::MessageLoop::current()->PostDelayedTask(
        FROM_HERE, base::Bind(&BenchmarkApp::StartTracedApplication,
                              base::Unretained(this)),
        base::TimeDelta::FromSeconds(1));
  }

  // Computes the string of trace categories we want to collect: a union of all
  // categories targeted in measurements.
  std::string ComputeCategoriesStr() {
    std::set<std::string> category_set;
    for (const Measurement& measurement : args_.measurements) {
      std::vector<std::string> categories =
          base::SplitString(measurement.target_event.categories, ",",
                            base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
      category_set.insert(categories.begin(), categories.end());
    }
    std::vector<std::string> unique_categories(category_set.begin(),
                                               category_set.end());
    return base::JoinString(unique_categories, ",");
  }

  void StartTracedApplication() {
    // Record the time origin for measurements just before connecting to the app
    // being benchmarked.
    time_origin_ = base::TimeTicks::FromInternalValue(MojoGetTimeTicksNow());
    shell()->ConnectToApplication(args_.app, GetProxy(&traced_app_connection_));

    // Post task to stop tracing when the time is up.
    base::MessageLoop::current()->PostDelayedTask(
        FROM_HERE,
        base::Bind(&BenchmarkApp::StopTracing, base::Unretained(this)),
        args_.duration);
  }

  void StopTracing() {
    // Request the trace collector to send back the data. When the data is ready
    // we will be called at OnTraceCollected().
    trace_collector_client_->Stop();
  }

  // TraceCollectorClient::Receiver:
  void OnTraceCollected(std::string trace_data) override {
    if (args_.write_output_file) {
      // Write the trace file regardless of whether it can be parsed (or whether
      // the measurements succeed), as it can be useful to debug failures.
      base::File trace_file(
          args_.output_file_path,
          base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
      trace_file.WriteAtCurrentPos(trace_data.data(), trace_data.size());
      printf("wrote trace file at: %s\n",
             args_.output_file_path.value().c_str());
    }

    // Parse trace events.
    std::vector<Event> events;
    if (!GetEvents(trace_data, &events)) {
      LOG(ERROR) << "Failed to parse the trace data";
      mojo::TerminateApplication(MOJO_RESULT_UNKNOWN);
      return;
    }

    // Calculate and print the results.
    bool succeeded = true;
    Measurements measurements(events, time_origin_);
    for (const Measurement& measurement : args_.measurements) {
      double result = measurements.Measure(measurement);
      if (result >= 0.0) {
        printf("measurement: %s %lf\n", measurement.spec.c_str(), result);
      } else {
        succeeded = false;
        printf("measurement: %s FAILED\n", measurement.spec.c_str());
      }
    }

    // Scripts that run benchmarks can pick this up as a signal that the run
    // succeeded, as shell exit code is 0 even if an app exits early due to an
    // error.
    if (succeeded) {
      printf("benchmark succeeded\n");
    } else {
      printf("some measurements failed\n");
    }
    mojo::TerminateApplication(MOJO_RESULT_OK);
  }

 private:
  RunArgs args_;
  mojo::InterfaceHandle<mojo::ServiceProvider> traced_app_connection_;
  scoped_ptr<TraceCollectorClient> trace_collector_client_;
  base::TimeTicks time_origin_;

  DISALLOW_COPY_AND_ASSIGN(BenchmarkApp);
};
}  // namespace
}  // namespace benchmark

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ScopedChromiumInit init;
  benchmark::BenchmarkApp benchmark_app;
  auto ret = mojo::RunApplication(application_request, &benchmark_app);
  fflush(nullptr);
  return ret;
}
