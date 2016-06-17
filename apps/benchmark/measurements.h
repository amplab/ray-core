// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPS_BENCHMARK_MEASUREMENTS_HH_
#define APPS_BENCHMARK_MEASUREMENTS_HH_

#include <vector>

#include "apps/benchmark/event.h"
#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/time/time.h"
#include "base/values.h"

namespace benchmark {

// Describes a trace event to match.
struct EventSpec {
  std::string name;
  std::string categories;

  EventSpec();
  EventSpec(std::string name, std::string categories);
  ~EventSpec();
};

enum class MeasurementType {
  TIME_UNTIL,
  TIME_BETWEEN,
  AVG_DURATION,
  PERCENTILE_DURATION,
};

// Represents a single measurement to be performed on the collected trace.
struct Measurement {
  MeasurementType type;
  EventSpec target_event;
  // Second event targeted by the measurement, meaningful only for binary
  // measurement types (TIME_BETWEEN).
  EventSpec second_event;
  // Optional parameter to the measurement.
  double param;
  // Optional string from which this measurement was parsed. Can be used for
  // presentation purposes.
  std::string spec;

  Measurement();
  Measurement(MeasurementType type, EventSpec target_event);
  Measurement(MeasurementType type,
              EventSpec target_event,
              EventSpec second_event);
  ~Measurement();
};

class Measurements {
 public:
  Measurements(std::vector<Event> events, base::TimeTicks time_origin);
  ~Measurements();

  // Performs the given measurement. Returns the result in milliseconds or -1.0
  // if the measurement failed, e.g. because no events were matched.
  double Measure(const Measurement& measurement) const;

 private:
  bool EarliestOccurence(const EventSpec& event_spec,
                         base::TimeTicks* earliest) const;
  double TimeUntil(const EventSpec& event_spec) const;
  double TimeBetween(const EventSpec& first_event_spec,
                     const EventSpec& second_event_spec) const;
  double AvgDuration(const EventSpec& event_spec) const;
  double Percentile(const EventSpec& event_spec, double percentile) const;

  std::vector<Event> events_;
  base::TimeTicks time_origin_;
  DISALLOW_COPY_AND_ASSIGN(Measurements);
};

}  // namespace benchmark

#endif  // APPS_BENCHMARK_MEASUREMENTS_HH_
