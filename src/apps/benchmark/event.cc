// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/benchmark/event.h"

#include <map>
#include <stack>

#include "base/json/json_reader.h"
#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"

namespace benchmark {

Event::Event() {}

Event::Event(EventType type,
             std::string name,
             std::string categories,
             base::TimeTicks timestamp,
             base::TimeDelta duration)
    : type(type),
      name(name),
      categories(categories),
      timestamp(timestamp),
      duration(duration) {}

Event::~Event() {}

namespace {
// ID uniquely identifying an asynchronous event stack.
struct AsyncEventStackId {
  std::string id;
  std::string cat;
};

bool operator<(const AsyncEventStackId& t, const AsyncEventStackId& o) {
  return t.id < o.id || (t.id == o.id && t.cat < o.cat);
}

// ID uniquely identifying a duration event stack.
typedef std::string DurationEventStackId;

bool ExtractKeyAsString(base::DictionaryValue* event_dict,
                        const std::string& key,
                        std::string* output) {
  const base::Value* value;
  if (!event_dict->Get(key, &value)) {
    LOG(ERROR) << "Incorrect trace event (missing " + key + "): "
               << *event_dict;
    return false;
  }

  if (value->IsType(base::Value::TYPE_INTEGER)) {
    int id_int = 0;
    // We already verified the type, so it should be an integer.
    CHECK(value->GetAsInteger(&id_int));
    *output = base::IntToString(id_int);
  } else if (value->IsType(base::Value::TYPE_STRING)) {
    CHECK(value->GetAsString(output));
  } else {
    LOG(ERROR) << "Incorrect trace event (" + key +
                      " not a string or integer): "
               << *event_dict;
    return false;
  }
  return true;
}

// Makes a unique id for a duration event stack.
bool GetDurationEventStackId(base::DictionaryValue* event_dict,
                             DurationEventStackId* durationId) {
  return ExtractKeyAsString(event_dict, "tid", durationId);
}

// Makes a unique key for an async event stack.
bool GetAsyncEventStackId(base::DictionaryValue* event_dict,
                          AsyncEventStackId* asyncId) {
  if (!ExtractKeyAsString(event_dict, "id", &asyncId->id)) {
    return false;
  }

  // We can have an empty category, but it is still relevant for event
  // merging per the documentation.
  std::string cat;
  event_dict->GetString("cat", &asyncId->cat);

  return true;
}

// Given a beginning event, registers its beginning for future merging.
template <typename T>
void RegisterEventBegin(
    T key,
    base::DictionaryValue* event_dict,
    std::map<T, std::stack<base::DictionaryValue*>>* open_events) {
  (*open_events)[key].push(event_dict);
}

// Given an end event, merges it with a previously-seen beginning event.
template <typename T>
bool MergeEventEnd(
    T key,
    base::DictionaryValue* event_dict,
    std::map<T, std::stack<base::DictionaryValue*>>* open_events) {
  if (!open_events->count(key) || (*open_events)[key].empty()) {
    LOG(ERROR) << "Incorrect trace event (event end without begin).";
    return false;
  }

  base::DictionaryValue* begin_event_dict = (*open_events)[key].top();
  (*open_events)[key].pop();
  double begin_ts;
  if (!begin_event_dict->GetDouble("ts", &begin_ts)) {
    LOG(ERROR) << "Incorrect trace event (no timestamp)";
    return false;
  }

  double end_ts;
  if (!event_dict->GetDouble("ts", &end_ts)) {
    LOG(ERROR) << "Incorrect trace event (no timestamp)";
    return false;
  }

  if (end_ts < begin_ts) {
    LOG(ERROR) << "Incorrect trace event (event ends before it begins)";
    return false;
  }

  begin_event_dict->SetDouble("dur", end_ts - begin_ts);
  begin_event_dict->SetString("ph", "X");
  return true;
}

// Finds the matching "end" event for each "begin" event of type duration or
// async, and rewrites the "begin event" as a "complete" event with a duration.
// Leaves all events that are not "begin" unchanged.
bool JoinEvents(base::ListValue* event_list) {
  // Maps thread ids to stacks of unmatched duration begin events on the given
  // thread.
  std::map<DurationEventStackId, std::stack<base::DictionaryValue*>>
      open_begin_duration_events;
  std::map<AsyncEventStackId, std::stack<base::DictionaryValue*>>
      open_begin_async_events;

  for (base::Value* val : *event_list) {
    base::DictionaryValue* event_dict;
    if (!val->GetAsDictionary(&event_dict))
      return false;

    std::string phase;
    if (!event_dict->GetString("ph", &phase)) {
      LOG(ERROR) << "Incorrect trace event (missing phase)";
      return false;
    }

    if (phase == "B" || phase == "E") {
      DurationEventStackId durationId;
      if (!GetDurationEventStackId(event_dict, &durationId)) {
        return false;
      }
      if (phase == "B") {
        RegisterEventBegin(durationId, event_dict, &open_begin_duration_events);
      } else if (phase == "E" &&
                 !MergeEventEnd(durationId, event_dict,
                                &open_begin_duration_events)) {
        return false;
      }
    } else if (phase == "b" || phase == "S" || phase == "e" || phase == "F") {
      AsyncEventStackId asyncId;
      if (!GetAsyncEventStackId(event_dict, &asyncId)) {
        return false;
      }
      if (phase == "b" || phase == "S") {
        RegisterEventBegin(asyncId, event_dict, &open_begin_async_events);
      } else if ((phase == "e" || phase == "F") &&
                 !MergeEventEnd(asyncId, event_dict,
                                &open_begin_async_events)) {
        return false;
      }
    }
  }
  return true;
}

bool ParseEvents(base::ListValue* event_list, std::vector<Event>* result) {
  result->clear();

  for (base::Value* val : *event_list) {
    base::DictionaryValue* event_dict;
    if (!val->GetAsDictionary(&event_dict)) {
      LOG(WARNING) << "Ignoring incorrect trace event (not a dictionary)";
      LOG(WARNING) << *event_dict;
      continue;
    }

    Event event;

    std::string phase;
    if (!event_dict->GetString("ph", &phase)) {
      LOG(WARNING) << "Ignoring incorrect trace event (missing phase)";
      LOG(WARNING) << *event_dict;
      continue;
    }
    if (phase == "X") {
      event.type = EventType::COMPLETE;
    } else if (phase == "I" || phase == "n") {
      event.type = EventType::INSTANT;
    } else {
      // Skip all event types we do not handle.
      continue;
    }

    if (!event_dict->GetString("name", &event.name)) {
      LOG(ERROR) << "Incorrect trace event (no name)";
      LOG(ERROR) << *event_dict;
      return false;
    }

    // Some clients do not add categories to events, but we don't want to fail
    // nor skip the event.
    event_dict->GetString("cat", &event.categories);

    double timestamp;
    if (!event_dict->GetDouble("ts", &timestamp)) {
      LOG(WARNING) << "Ingoring incorrect trace event (no timestamp)";
      LOG(WARNING) << *event_dict;
      continue;
    }
    event.timestamp = base::TimeTicks::FromInternalValue(timestamp);

    if (event.type == EventType::COMPLETE) {
      double duration;
      if (!event_dict->GetDouble("dur", &duration)) {
        LOG(WARNING) << "Ignoring incorrect complete event (no duration)";
        LOG(WARNING) << *event_dict;
        continue;
      }

      event.duration = base::TimeDelta::FromInternalValue(duration);
    } else {
      event.duration = base::TimeDelta();
    }

    result->push_back(event);
  }
  return true;
}

}  // namespace

bool GetEvents(const std::string& trace_json, std::vector<Event>* result) {
  // Parse the JSON string describing the events.
  base::JSONReader reader;
  scoped_ptr<base::Value> trace_data = reader.ReadToValue(trace_json);
  if (!trace_data) {
    LOG(ERROR) << "Failed to parse the JSON string: "
               << reader.GetErrorMessage();
    return false;
  }

  base::ListValue* event_list;
  if (!trace_data->GetAsList(&event_list)) {
    LOG(ERROR) << "Incorrect format of the trace data.";
    return false;
  }

  if (!JoinEvents(event_list))
    return false;

  if (!ParseEvents(event_list, result))
    return false;
  return true;
}
}  // namespace benchmark
