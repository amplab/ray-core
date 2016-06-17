// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPS_BENCHMARK_RUN_ARGS_H_
#define APPS_BENCHMARK_RUN_ARGS_H_

#include <string>
#include <vector>

#include "apps/benchmark/measurements.h"
#include "base/files/file_path.h"
#include "base/time/time.h"

namespace benchmark {

// Represents arguments for a run of the benchmark app.
struct RunArgs {
  std::string app;
  base::TimeDelta duration;
  std::vector<Measurement> measurements;
  bool write_output_file;
  base::FilePath output_file_path;

  RunArgs();
  ~RunArgs();
};

// Parses the arguments representation from the format provided by
// ApplicationImpl::args(). Returns true iff the arguments were correctly parsed
// and stored in |result|.
bool GetRunArgs(const std::vector<std::string>& input_args, RunArgs* result);

}  // namespace benchmark

#endif  // APPS_BENCHMARK_RUN_ARGS_H_
