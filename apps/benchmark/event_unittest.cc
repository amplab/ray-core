// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/benchmark/event.h"

#include <string>
#include <vector>

#include "base/strings/string_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace benchmark {

namespace {

TEST(GetEventsTest, Empty) {
  std::vector<Event> events;
  EXPECT_TRUE(GetEvents("[]", &events));
  EXPECT_EQ(0u, events.size());
}

TEST(GetEventsTest, Invalid) {
  std::vector<Event> events;
  EXPECT_FALSE(GetEvents("", &events));
  EXPECT_FALSE(GetEvents("[", &events));
  EXPECT_FALSE(GetEvents("]", &events));
  EXPECT_FALSE(GetEvents("[,]", &events));
}

TEST(GetEventsTest, Typical) {
  std::string event1 =
      "{\"pid\":7845,\"tid\":7845,\"ts\":1988539886444,"
      "\"ph\":\"X\",\"cat\":\"toplevel\",\"name\":\"MessageLoop::RunTask\","
      "\"args\":{\"src_file\":\"../../mojo/message_pump/handle_watcher.cc\","
      "\"src_func\":\"RemoveAndNotify\"},\"dur\":647,\"tdur\":633,"
      "\"tts\":25295}";
  std::string event2 =
      "{\"pid\":8238,\"tid\":8264,\"ts\":1988975370433,\"ph\":\"X\","
      "\"cat\":\"gpu\",\"name\":\"GLES2::WaitForCmd\",\"args\":{},\"dur\":7288,"
      "\"tdur\":212,\"tts\":6368}";

  std::string trace_json = "[" + event1 + "," + event2 + "]";
  std::vector<Event> events;
  ASSERT_TRUE(GetEvents(trace_json, &events));
  ASSERT_EQ(2u, events.size());

  EXPECT_EQ(EventType::COMPLETE, events[0].type);
  EXPECT_EQ("MessageLoop::RunTask", events[0].name);
  EXPECT_EQ("toplevel", events[0].categories);
  EXPECT_EQ(base::TimeTicks::FromInternalValue(1988539886444),
            events[0].timestamp);
  EXPECT_EQ(base::TimeDelta::FromInternalValue(647), events[0].duration);

  EXPECT_EQ(EventType::COMPLETE, events[1].type);
  EXPECT_EQ("GLES2::WaitForCmd", events[1].name);
  EXPECT_EQ("gpu", events[1].categories);
  EXPECT_EQ(base::TimeTicks::FromInternalValue(1988975370433),
            events[1].timestamp);
  EXPECT_EQ(base::TimeDelta::FromInternalValue(7288), events[1].duration);
}

TEST(GetEventsTest, InstantEvent) {
  std::string event =
      "{\"pid\":19309,\"tid\":8,\"ts\":2949822399148,\"ph\":\"I\","
      "\"cat\":\"cc\",\"name\":\"Scheduler::BeginRetroFrames all expired\","
      "\"args\":{},\"tts\":375966,\"s\":\"t\"}";

  std::string trace_json = "[" + event + "]";
  std::vector<Event> events;
  ASSERT_TRUE(GetEvents(trace_json, &events));
  ASSERT_EQ(1u, events.size());

  EXPECT_EQ(EventType::INSTANT, events[0].type);
  EXPECT_EQ("Scheduler::BeginRetroFrames all expired", events[0].name);
  EXPECT_EQ("cc", events[0].categories);
  EXPECT_EQ(base::TimeTicks::FromInternalValue(2949822399148),
            events[0].timestamp);
  EXPECT_EQ(base::TimeDelta::FromInternalValue(0), events[0].duration);
}

TEST(GetEventsTest, DurationEventsSameThread) {
  std::vector<std::string> event_specs(6);
  event_specs[0] =
      "{\"tid\":1,\"ts\":1,\"ph\":\"B\",\"cat\":\"cc\",\"name\":\"Outer "
      "event\"}";
  event_specs[1] =
      "{\"tid\":1,\"ts\":2,\"ph\":\"B\",\"cat\":\"cc\",\"name\":\"Inner "
      "event\"}";
  event_specs[2] = "{\"tid\":1,\"ts\":3,\"ph\":\"E\"}";
  event_specs[3] = "{\"tid\":1,\"ts\":4,\"ph\":\"E\"}";
  event_specs[4] =
      "{\"tid\":1,\"ts\":5,\"ph\":\"B\",\"cat\":\"cc\",\"name\":\"Next "
      "event\"}";
  event_specs[5] = "{\"tid\":1,\"ts\":6,\"ph\":\"E\"}";

  std::string trace_json = "[" + base::JoinString(event_specs, ",") + "]";
  std::vector<Event> events;
  ASSERT_TRUE(GetEvents(trace_json, &events));
  ASSERT_EQ(3u, events.size());

  EXPECT_EQ(EventType::COMPLETE, events[0].type);
  EXPECT_EQ("Outer event", events[0].name);
  EXPECT_EQ("cc", events[0].categories);
  EXPECT_EQ(base::TimeTicks::FromInternalValue(1), events[0].timestamp);
  EXPECT_EQ(base::TimeDelta::FromInternalValue(3), events[0].duration);

  EXPECT_EQ(EventType::COMPLETE, events[1].type);
  EXPECT_EQ("Inner event", events[1].name);
  EXPECT_EQ("cc", events[1].categories);
  EXPECT_EQ(base::TimeTicks::FromInternalValue(2), events[1].timestamp);
  EXPECT_EQ(base::TimeDelta::FromInternalValue(1), events[1].duration);

  EXPECT_EQ(EventType::COMPLETE, events[2].type);
  EXPECT_EQ("Next event", events[2].name);
  EXPECT_EQ("cc", events[2].categories);
  EXPECT_EQ(base::TimeTicks::FromInternalValue(5), events[2].timestamp);
  EXPECT_EQ(base::TimeDelta::FromInternalValue(1), events[2].duration);
}

TEST(GetEventsTest, DurationEventsTwoThreads) {
  std::vector<std::string> event_specs(4);
  event_specs[0] =
      "{\"tid\":1,\"ts\":1,\"ph\":\"B\",\"cat\":\"cc\",\"name\":\"t1 event\"}";
  event_specs[1] =
      "{\"tid\":2,\"ts\":2,\"ph\":\"B\",\"cat\":\"cc\",\"name\":\"t2 event\"}";
  event_specs[2] = "{\"tid\":1,\"ts\":3,\"ph\":\"E\"}";
  event_specs[3] = "{\"tid\":2,\"ts\":4,\"ph\":\"E\"}";

  std::string trace_json = "[" + base::JoinString(event_specs, ",") + "]";
  std::vector<Event> events;
  ASSERT_TRUE(GetEvents(trace_json, &events));
  ASSERT_EQ(2u, events.size());

  EXPECT_EQ(EventType::COMPLETE, events[0].type);
  EXPECT_EQ("t1 event", events[0].name);
  EXPECT_EQ("cc", events[0].categories);
  EXPECT_EQ(base::TimeTicks::FromInternalValue(1), events[0].timestamp);
  EXPECT_EQ(base::TimeDelta::FromInternalValue(2), events[0].duration);

  EXPECT_EQ(EventType::COMPLETE, events[1].type);
  EXPECT_EQ("t2 event", events[1].name);
  EXPECT_EQ("cc", events[1].categories);
  EXPECT_EQ(base::TimeTicks::FromInternalValue(2), events[1].timestamp);
  EXPECT_EQ(base::TimeDelta::FromInternalValue(2), events[1].duration);
}

TEST(GetEventsTest, DurationEventsTidIsString) {
  std::vector<std::string> event_specs(2);
  event_specs[0] =
      "{\"tid\":\"1\",\"ts\":1,\"ph\":\"B\",\"cat\":\"cc\","
      "\"name\":\"t1 event\"}";
  event_specs[1] = "{\"tid\":\"1\",\"ts\":3,\"ph\":\"E\"}";

  std::string trace_json = "[" + base::JoinString(event_specs, ",") + "]";
  std::vector<Event> events;
  ASSERT_TRUE(GetEvents(trace_json, &events));
  ASSERT_EQ(1u, events.size());

  EXPECT_EQ(EventType::COMPLETE, events[0].type);
  EXPECT_EQ("t1 event", events[0].name);
  EXPECT_EQ("cc", events[0].categories);
  EXPECT_EQ(base::TimeTicks::FromInternalValue(1), events[0].timestamp);
  EXPECT_EQ(base::TimeDelta::FromInternalValue(2), events[0].duration);
}

TEST(GetEventsTest, AsyncEvents) {
  std::vector<std::string> event_specs(4);
  event_specs[0] =
      "{\"tid\":1001,\"id\":1,\"ts\":1,\"ph\":\"S\",\"cat\":\"cc\",\"name\":"
      "\"t1 event\"}";
  event_specs[1] =
      "{\"tid\":1002,\"id\":2,\"ts\":2,\"ph\":\"S\",\"cat\":\"cc\",\"name\":"
      "\"t2 event\"}";
  event_specs[2] =
      "{\"tid\":1003,\"id\":1,\"ts\":3,\"ph\":\"F\",\"cat\":\"cc\",\"name\":"
      "\"t1 event\"}";
  event_specs[3] =
      "{\"tid\":1004,\"id\":2,\"ts\":4,\"ph\":\"F\",\"cat\":\"cc\",\"name\":"
      "\"t2 event\"}";

  std::string trace_json = "[" + base::JoinString(event_specs, ",") + "]";
  std::vector<Event> events;
  ASSERT_TRUE(GetEvents(trace_json, &events));
  ASSERT_EQ(2u, events.size());

  EXPECT_EQ(EventType::COMPLETE, events[0].type);
  EXPECT_EQ("t1 event", events[0].name);
  EXPECT_EQ("cc", events[0].categories);
  EXPECT_EQ(base::TimeTicks::FromInternalValue(1), events[0].timestamp);
  EXPECT_EQ(base::TimeDelta::FromInternalValue(2), events[0].duration);

  EXPECT_EQ(EventType::COMPLETE, events[1].type);
  EXPECT_EQ("t2 event", events[1].name);
  EXPECT_EQ("cc", events[1].categories);
  EXPECT_EQ(base::TimeTicks::FromInternalValue(2), events[1].timestamp);
  EXPECT_EQ(base::TimeDelta::FromInternalValue(2), events[1].duration);
}

TEST(GetEventsTest, AsyncEventIdIsString) {
  std::vector<std::string> event_specs(2);
  event_specs[0] =
      "{\"tid\":1001,\"id\":\"a\",\"ts\":1,\"ph\":\"S\",\"cat\":\"cc\","
      "\"name\":\"t1 event\"}";
  event_specs[1] =
      "{\"tid\":1003,\"id\":\"a\",\"ts\":3,\"ph\":\"F\",\"cat\":\"cc\","
      "\"name\":\"t1 event\"}";

  std::string trace_json = "[" + base::JoinString(event_specs, ",") + "]";
  std::vector<Event> events;
  ASSERT_TRUE(GetEvents(trace_json, &events));
  ASSERT_EQ(1u, events.size());

  EXPECT_EQ(EventType::COMPLETE, events[0].type);
  EXPECT_EQ("t1 event", events[0].name);
  EXPECT_EQ("cc", events[0].categories);
  EXPECT_EQ(base::TimeTicks::FromInternalValue(1), events[0].timestamp);
  EXPECT_EQ(base::TimeDelta::FromInternalValue(2), events[0].duration);
}

}  // namespace

}  // namespace benchmark
