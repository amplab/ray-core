// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/benchmark/measurements.h"

#include <algorithm>

namespace benchmark {
namespace {

static bool Match(const Event& event, const EventSpec& spec) {
  return event.name == spec.name && event.categories == spec.categories;
}

}  // namespace

EventSpec::EventSpec() {}

EventSpec::EventSpec(std::string name, std::string categories)
    : name(name), categories(categories) {}

EventSpec::~EventSpec() {}

Measurement::Measurement() {}

Measurement::Measurement(MeasurementType type, EventSpec target_event)
    : type(type), target_event(target_event) {}

Measurement::Measurement(MeasurementType type,
                         EventSpec target_event,
                         EventSpec second_event)
    : type(type), target_event(target_event), second_event(second_event) {}

Measurement::~Measurement() {}

Measurements::Measurements(std::vector<Event> events,
                           base::TimeTicks time_origin)
    : events_(events), time_origin_(time_origin) {}

Measurements::~Measurements() {}

double Measurements::Measure(const Measurement& measurement) const {
  switch (measurement.type) {
    case MeasurementType::TIME_UNTIL:
      return TimeUntil(measurement.target_event);
    case MeasurementType::TIME_BETWEEN:
      return TimeBetween(measurement.target_event, measurement.second_event);
    case MeasurementType::AVG_DURATION:
      return AvgDuration(measurement.target_event);
    case MeasurementType::PERCENTILE_DURATION:
      return Percentile(measurement.target_event, measurement.param);
    default:
      NOTREACHED();
      return double();
  }
}

bool Measurements::EarliestOccurence(const EventSpec& event_spec,
                                     base::TimeTicks* earliest) const {
  base::TimeTicks result;
  bool found = false;
  for (const Event& event : events_) {
    if (!Match(event, event_spec))
      continue;

    if (found) {
      result = std::min(result, event.timestamp);
    } else {
      result = event.timestamp;
      found = true;
    }
  }
  if (!found)
    return false;
  *earliest = result;
  return true;
}

double Measurements::TimeUntil(const EventSpec& event_spec) const {
  base::TimeTicks earliest;
  if (!EarliestOccurence(event_spec, &earliest))
    return -1.0;
  return (earliest - time_origin_).InMillisecondsF();
}

double Measurements::TimeBetween(const EventSpec& first_event_spec,
                                 const EventSpec& second_event_spec) const {
  base::TimeTicks earliest_first_event;
  if (!EarliestOccurence(first_event_spec, &earliest_first_event))
    return -1.0;
  base::TimeTicks earliest_second_event;
  if (!EarliestOccurence(second_event_spec, &earliest_second_event))
    return -1.0;
  if (earliest_second_event < earliest_first_event)
    return -1.0;
  return (earliest_second_event - earliest_first_event).InMillisecondsF();
}

double Measurements::AvgDuration(const EventSpec& event_spec) const {
  double sum = 0.0;
  int count = 0;
  for (const Event& event : events_) {
    if (event.type != EventType::COMPLETE)
      continue;

    if (!Match(event, event_spec))
      continue;

    sum += event.duration.InMillisecondsF();
    count += 1;
  }

  if (!count)
    return -1.0;
  return sum / count;
}

double Measurements::Percentile(const EventSpec& event_spec,
                                double percentile) const {
  DCHECK_GE(percentile, 0.0);
  DCHECK_LE(percentile, 1.0);
  std::vector<double> durations;

  for (const Event& event : events_) {
    if (event.type != EventType::COMPLETE)
      continue;

    if (!Match(event, event_spec))
      continue;

    durations.push_back(event.duration.InMillisecondsF());
  }
  if (durations.size() == 0) {
    return -1.0;
  }

  // Nearest-rank method from:
  // https://en.wikipedia.org/wiki/Percentile
  double size = static_cast<double>(durations.size());
  size_t rank = static_cast<size_t>(ceil(size * percentile));
  size_t index = std::max(size_t{1}, rank) - 1;
  std::nth_element(durations.begin(), durations.begin() + index,
                   durations.end());
  return durations[index];
}

}  // namespace benchmark
