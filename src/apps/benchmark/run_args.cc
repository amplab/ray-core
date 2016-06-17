// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/benchmark/run_args.h"

#include <algorithm>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"

namespace benchmark {
namespace {

bool CheckMeasurementFormat(const std::string& type,
                            int required_args,
                            int provided_args) {
  if (required_args != provided_args) {
    LOG(ERROR) << "Could not parse a measurement of type " << type
               << ", expected " << required_args << " while " << provided_args
               << " were provided";
    return false;
  }
  return true;
}

bool GetMeasurement(const std::string& measurement_spec, Measurement* result) {
  // Measurements are described in the format:
  // <measurement type>/<event category>/<event name>.
  std::vector<std::string> parts = base::SplitString(
      measurement_spec, "/", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  if (parts.size() < 1) {
    LOG(ERROR) << "Could not parse the measurement description: "
               << measurement_spec;
    return false;
  }

  if (parts[0] == "time_until") {
    if (!CheckMeasurementFormat(parts[0], 3, parts.size()))
      return false;
    *result =
        Measurement(MeasurementType::TIME_UNTIL, EventSpec(parts[2], parts[1]));
  } else if (parts[0] == "time_between") {
    if (!CheckMeasurementFormat(parts[0], 5, parts.size()))
      return false;
    *result = Measurement(MeasurementType::TIME_BETWEEN,
                          EventSpec(parts[2], parts[1]),
                          EventSpec(parts[4], parts[3]));
  } else if (parts[0] == "avg_duration") {
    if (!CheckMeasurementFormat(parts[0], 3, parts.size()))
      return false;
    *result = Measurement(MeasurementType::AVG_DURATION,
                          EventSpec(parts[2], parts[1]));
  } else if (parts[0] == "percentile_duration") {
    if (!CheckMeasurementFormat(parts[0], 4, parts.size()))
      return false;
    *result = Measurement(MeasurementType::PERCENTILE_DURATION,
                          EventSpec(parts[2], parts[1]));
    if (!base::StringToDouble(parts[3], &result->param)) {
      LOG(ERROR) << "Expected '" << parts[3]
                 << "'' to be a number in: " << measurement_spec;
      return false;
    }
    if ((result->param < 0.0) || (result->param > 1.0)) {
      LOG(ERROR) << "Expected '" << result->param << "' to be >=0.0 and <=1.0 "
                 << "in " << measurement_spec;
    }
  } else {
    LOG(ERROR) << "Could not recognize the measurement type: " << parts[0];
    return false;
  }

  result->spec = measurement_spec;
  return true;
}

}  // namespace

RunArgs::RunArgs() {}

RunArgs::~RunArgs() {}

bool GetRunArgs(const std::vector<std::string>& input_args, RunArgs* result) {
  base::CommandLine command_line(input_args);

  result->app = command_line.GetSwitchValueASCII("app");
  if (result->app.empty()) {
    LOG(ERROR) << "The required --app argument is empty or missing.";
    return false;
  }

  std::string duration_str = command_line.GetSwitchValueASCII("duration");
  if (duration_str.empty()) {
    LOG(ERROR) << "The required --duration argument is empty or missing.";
    return false;
  }
  int duration_int;
  if (!base::StringToInt(duration_str, &duration_int) || duration_int <= 0) {
    LOG(ERROR) << "Could not parse the --duration value as a positive integer: "
               << duration_str;
    return false;
  }
  result->duration = base::TimeDelta::FromSeconds(duration_int);

  result->write_output_file = false;
  if (command_line.HasSwitch("trace-output")) {
    result->write_output_file = true;
    result->output_file_path =
        base::FilePath(command_line.GetSwitchValueASCII("trace-output"));
  }

  // All regular arguments (not switches, ie. not preceded by "--") describe
  // measurements.
  for (const std::string& measurement_spec : command_line.GetArgs()) {
    Measurement measurement;
    std::string unquoted_measurement_spec;
    base::TrimString(measurement_spec, "'\"", &unquoted_measurement_spec);
    if (!GetMeasurement(unquoted_measurement_spec, &measurement)) {
      return false;
    }
    result->measurements.push_back(measurement);
  }

  return true;
}

}  // namespace benchmark
