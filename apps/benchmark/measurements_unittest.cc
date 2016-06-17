// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/benchmark/measurements.h"

#include <algorithm>

#include "testing/gtest/include/gtest/gtest.h"

namespace benchmark {
namespace {

base::TimeTicks Ticks(int64 value) {
  return base::TimeTicks::FromInternalValue(value);
}

base::TimeDelta Delta(int64 value) {
  return base::TimeDelta::FromInternalValue(value);
}

class MeasurementsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    events_.resize(11);
    events_[0] = Event(EventType::COMPLETE, "a", "some", Ticks(10), Delta(2));
    events_[1] = Event(EventType::COMPLETE, "a", "some", Ticks(11), Delta(4));
    events_[2] = Event(EventType::COMPLETE, "a", "other", Ticks(12), Delta(8));
    events_[3] = Event(EventType::COMPLETE, "b", "some", Ticks(3), Delta(16));
    events_[4] = Event(EventType::COMPLETE, "b", "some", Ticks(13), Delta(32));

    events_[5] = Event(EventType::COMPLETE, "c", "some", Ticks(14), Delta(10));
    events_[6] = Event(EventType::COMPLETE, "c", "some", Ticks(16), Delta(11));
    events_[7] = Event(EventType::COMPLETE, "c", "some", Ticks(18), Delta(12));

    events_[8] =
        Event(EventType::INSTANT, "instant", "another", Ticks(20), Delta(0));
    events_[9] = Event(EventType::INSTANT, "multi_occurence", "another",
                       Ticks(30), Delta(0));
    events_[10] = Event(EventType::INSTANT, "multi_occurence", "another",
                        Ticks(40), Delta(0));

    reversed_ = events_;
    reverse(reversed_.begin(), reversed_.end());
  }

  std::vector<Event> events_;
  std::vector<Event> reversed_;
};

TEST_F(MeasurementsTest, MeasureTimeUntil) {
  // The results should be the same regardless of the order of events.
  Measurements regular(events_, base::TimeTicks::FromInternalValue(2));
  Measurements reversed(reversed_, base::TimeTicks::FromInternalValue(2));

  EXPECT_DOUBLE_EQ(0.008,
                   regular.Measure(Measurement(MeasurementType::TIME_UNTIL,
                                               EventSpec("a", "some"))));
  EXPECT_DOUBLE_EQ(0.008,
                   reversed.Measure(Measurement(MeasurementType::TIME_UNTIL,
                                                EventSpec("a", "some"))));

  EXPECT_DOUBLE_EQ(0.01,
                   regular.Measure(Measurement(MeasurementType::TIME_UNTIL,
                                               EventSpec("a", "other"))));
  EXPECT_DOUBLE_EQ(0.01,
                   reversed.Measure(Measurement(MeasurementType::TIME_UNTIL,
                                                EventSpec("a", "other"))));

  EXPECT_DOUBLE_EQ(0.001,
                   regular.Measure(Measurement(MeasurementType::TIME_UNTIL,
                                               EventSpec("b", "some"))));
  EXPECT_DOUBLE_EQ(0.001,
                   reversed.Measure(Measurement(MeasurementType::TIME_UNTIL,
                                                EventSpec("b", "some"))));
}

TEST_F(MeasurementsTest, MeasureTimeBetween) {
  // The results should be the same regardless of the order of events.
  Measurements regular(events_, base::TimeTicks::FromInternalValue(2));
  Measurements reversed(reversed_, base::TimeTicks::FromInternalValue(2));

  EXPECT_DOUBLE_EQ(0.0, regular.Measure(Measurement(
                            MeasurementType::TIME_BETWEEN,
                            EventSpec("a", "some"), EventSpec("a", "some"))));
  EXPECT_DOUBLE_EQ(0.0, reversed.Measure(Measurement(
                            MeasurementType::TIME_BETWEEN,
                            EventSpec("a", "some"), EventSpec("a", "some"))));

  EXPECT_DOUBLE_EQ(
      0.01, regular.Measure(Measurement(
                MeasurementType::TIME_BETWEEN, EventSpec("instant", "another"),
                EventSpec("multi_occurence", "another"))));
  EXPECT_DOUBLE_EQ(
      0.01, reversed.Measure(Measurement(
                MeasurementType::TIME_BETWEEN, EventSpec("instant", "another"),
                EventSpec("multi_occurence", "another"))));

  EXPECT_DOUBLE_EQ(
      -1.0, regular.Measure(Measurement(MeasurementType::TIME_BETWEEN,
                                        EventSpec("multi_occurence", "another"),
                                        EventSpec("instant", "another"))));
  EXPECT_DOUBLE_EQ(-1.0, reversed.Measure(Measurement(
                             MeasurementType::TIME_BETWEEN,
                             EventSpec("multi_occurence", "another"),
                             EventSpec("instant", "another"))));

  EXPECT_DOUBLE_EQ(
      0.01, regular.Measure(Measurement(MeasurementType::TIME_BETWEEN,
                                        EventSpec("a", "some"),
                                        EventSpec("instant", "another"))));
  EXPECT_DOUBLE_EQ(
      0.01, reversed.Measure(Measurement(MeasurementType::TIME_BETWEEN,
                                         EventSpec("a", "some"),
                                         EventSpec("instant", "another"))));
}

TEST_F(MeasurementsTest, MeasureAvgDuration) {
  // The results should be the same regardless of the order of events.
  Measurements regular(events_, base::TimeTicks::FromInternalValue(2));
  Measurements reversed(reversed_, base::TimeTicks::FromInternalValue(2));

  EXPECT_DOUBLE_EQ(0.003,
                   regular.Measure(Measurement(MeasurementType::AVG_DURATION,
                                               EventSpec("a", "some"))));
  EXPECT_DOUBLE_EQ(0.003,
                   reversed.Measure(Measurement(MeasurementType::AVG_DURATION,
                                                EventSpec("a", "some"))));

  EXPECT_DOUBLE_EQ(0.008,
                   regular.Measure(Measurement(MeasurementType::AVG_DURATION,
                                               EventSpec("a", "other"))));
  EXPECT_DOUBLE_EQ(0.008,
                   reversed.Measure(Measurement(MeasurementType::AVG_DURATION,
                                                EventSpec("a", "other"))));

  EXPECT_DOUBLE_EQ(0.024,
                   regular.Measure(Measurement(MeasurementType::AVG_DURATION,
                                               EventSpec("b", "some"))));
  EXPECT_DOUBLE_EQ(0.024,
                   reversed.Measure(Measurement(MeasurementType::AVG_DURATION,
                                                EventSpec("b", "some"))));
}

TEST_F(MeasurementsTest, MeasurePercentileDuration) {
  // The results should be the same regardless of the order of events.
  Measurements regular(events_, base::TimeTicks::FromInternalValue(2));
  Measurements reversed(reversed_, base::TimeTicks::FromInternalValue(2));

  Measurement measurement(MeasurementType::PERCENTILE_DURATION,
                          EventSpec("c", "some"));
  measurement.param = 0.10;
  EXPECT_DOUBLE_EQ(0.010, regular.Measure(measurement));
  measurement.param = 0.50;
  EXPECT_DOUBLE_EQ(0.011, regular.Measure(measurement));
  measurement.param = 0.90;
  EXPECT_DOUBLE_EQ(0.012, regular.Measure(measurement));

  measurement.param = 0.10;
  EXPECT_DOUBLE_EQ(0.010, reversed.Measure(measurement));
  measurement.param = 0.50;
  EXPECT_DOUBLE_EQ(0.011, reversed.Measure(measurement));
  measurement.param = 0.90;
  EXPECT_DOUBLE_EQ(0.012, reversed.Measure(measurement));

  measurement.param = 1.0;
  EXPECT_DOUBLE_EQ(0.012, regular.Measure(measurement));
  measurement.param = 0.0;
  EXPECT_DOUBLE_EQ(0.010, regular.Measure(measurement));

  measurement.param = 1.0;
  EXPECT_DOUBLE_EQ(0.012, reversed.Measure(measurement));
  measurement.param = 0.0;
  EXPECT_DOUBLE_EQ(0.010, reversed.Measure(measurement));
}

TEST_F(MeasurementsTest, NoMatchingEvent) {
  // The results should be the same regardless of the order of events.
  Measurements empty(std::vector<Event>(),
                     base::TimeTicks::FromInternalValue(0));
  Measurements regular(events_, base::TimeTicks::FromInternalValue(0));
  Measurements reversed(reversed_, base::TimeTicks::FromInternalValue(0));

  EXPECT_DOUBLE_EQ(-1.0,
                   empty.Measure(Measurement(MeasurementType::AVG_DURATION,
                                             EventSpec("miss", "cat"))));
  EXPECT_DOUBLE_EQ(-1.0,
                   regular.Measure(Measurement(MeasurementType::AVG_DURATION,
                                               EventSpec("miss", "cat"))));
  EXPECT_DOUBLE_EQ(-1.0,
                   reversed.Measure(Measurement(MeasurementType::AVG_DURATION,
                                                EventSpec("miss", "cat"))));

  EXPECT_DOUBLE_EQ(-1.0, empty.Measure(Measurement(MeasurementType::TIME_UNTIL,
                                                   EventSpec("miss", "cat"))));
  EXPECT_DOUBLE_EQ(-1.0,
                   regular.Measure(Measurement(MeasurementType::TIME_UNTIL,
                                               EventSpec("miss", "cat"))));
  EXPECT_DOUBLE_EQ(-1.0,
                   reversed.Measure(Measurement(MeasurementType::TIME_UNTIL,
                                                EventSpec("miss", "cat"))));
}

}  // namespace

}  // namespace benchmark
