// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPS_BENCHMARK_EVENT_H_
#define APPS_BENCHMARK_EVENT_H_

#include <string>
#include <vector>

#include "base/time/time.h"
#include "base/values.h"

namespace benchmark {

// Event types as per the Trace Event format spec.
enum class EventType {COMPLETE, INSTANT};

// Represents a single trace event.
struct Event {
  EventType type;
  std::string name;
  std::string categories;
  base::TimeTicks timestamp;
  base::TimeDelta duration;

  Event();
  Event(EventType type,
        std::string name,
        std::string category,
        base::TimeTicks timestamp,
        base::TimeDelta duration);
  ~Event();
};

// Parses a JSON string representing a list of trace events. Stores the outcome
// in |result| and returns true on success, returns false on error.
//
// The supported event types as per the Trace Event format spec:
//  - duration events (phase "B" or "E")
//  - complete events (phase "X")
//  - instant events (phase "I")
//
//  All other types are accepted, but ignored, ie. they don't appear in
//  |result|. All duration events are joined to form the corresponding complete
//  events and the resulting complete events appear in |result| instead of the
//  the duration events.
bool GetEvents(const std::string& trace_json, std::vector<Event>* result);

}  // namespace benchmark
#endif  // APPS_BENCHMARK_EVENT_H_
