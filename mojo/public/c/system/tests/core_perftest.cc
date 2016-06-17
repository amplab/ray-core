// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This tests the performance of the C API.

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <functional>
#include <thread>

#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/message_pipe.h"
#include "mojo/public/c/system/result.h"
#include "mojo/public/c/system/time.h"
#include "mojo/public/c/system/wait.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/public/cpp/test_support/test_support.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

using PerfTestSingleIteration = std::function<void()>;

void IterateAndReportPerf(const char* test_name,
                          const char* sub_test_name,
                          PerfTestSingleIteration single_iteration) {
  // TODO(vtl): These should be specifiable using command-line flags.
  static const size_t kGranularity = 100u;
  static const MojoTimeTicks kPerftestTimeMicroseconds = 3 * 1000000;

  const MojoTimeTicks start_time = MojoGetTimeTicksNow();
  MojoTimeTicks end_time;
  size_t iterations = 0u;
  do {
    for (size_t i = 0u; i < kGranularity; i++)
      single_iteration();
    iterations += kGranularity;

    end_time = MojoGetTimeTicksNow();
  } while (end_time - start_time < kPerftestTimeMicroseconds);

  mojo::test::LogPerfResult(test_name, sub_test_name,
                            1000000.0 * iterations / (end_time - start_time),
                            "iterations/second");
}

class TestThread {
 public:
  void Start() {
    thread_ = std::thread([this]() { Run(); });
  }
  void Join() { thread_.join(); }

  virtual void Run() = 0;

 protected:
  TestThread() {}
  virtual ~TestThread() {}

 private:
  std::thread thread_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(TestThread);
};

class MessagePipeWriterThread : public TestThread {
 public:
  MessagePipeWriterThread(MojoHandle handle, uint32_t num_bytes)
      : handle_(handle), num_bytes_(num_bytes), num_writes_(0) {}
  ~MessagePipeWriterThread() override {}

  void Run() override {
    char buffer[10000];
    assert(num_bytes_ <= sizeof(buffer));

    // TODO(vtl): Should I throttle somehow?
    for (;;) {
      MojoResult result = MojoWriteMessage(handle_, buffer, num_bytes_, nullptr,
                                           0u, MOJO_WRITE_MESSAGE_FLAG_NONE);
      if (result == MOJO_RESULT_OK) {
        num_writes_++;
        continue;
      }

      // We failed to write.
      // Either |handle_| or its peer was closed.
      assert(result == MOJO_RESULT_INVALID_ARGUMENT ||
             result == MOJO_RESULT_FAILED_PRECONDITION);
      break;
    }
  }

  // Use only after joining the thread.
  int64_t num_writes() const { return num_writes_; }

 private:
  const MojoHandle handle_;
  const uint32_t num_bytes_;
  int64_t num_writes_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(MessagePipeWriterThread);
};

class MessagePipeReaderThread : public TestThread {
 public:
  explicit MessagePipeReaderThread(MojoHandle handle)
      : handle_(handle), num_reads_(0) {}
  ~MessagePipeReaderThread() override {}

  void Run() override {
    char buffer[10000];

    for (;;) {
      uint32_t num_bytes = static_cast<uint32_t>(sizeof(buffer));
      MojoResult result = MojoReadMessage(handle_, buffer, &num_bytes, nullptr,
                                          nullptr, MOJO_READ_MESSAGE_FLAG_NONE);
      if (result == MOJO_RESULT_OK) {
        num_reads_++;
        continue;
      }

      if (result == MOJO_RESULT_SHOULD_WAIT) {
        result = MojoWait(handle_, MOJO_HANDLE_SIGNAL_READABLE,
                          MOJO_DEADLINE_INDEFINITE, nullptr);
        if (result == MOJO_RESULT_OK) {
          // Go to the top of the loop to read again.
          continue;
        }
      }

      // We failed to read and possibly failed to wait.
      // Either |handle_| or its peer was closed.
      assert(result == MOJO_RESULT_INVALID_ARGUMENT ||
             result == MOJO_RESULT_FAILED_PRECONDITION);
      break;
    }
  }

  // Use only after joining the thread.
  int64_t num_reads() const { return num_reads_; }

 private:
  const MojoHandle handle_;
  int64_t num_reads_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(MessagePipeReaderThread);
};

class CorePerftest : public testing::Test {
 public:
  CorePerftest() {}
  ~CorePerftest() override {}

  void NoOp() {}

  void MessagePipe_CreateAndClose() {
    MojoResult result = MojoCreateMessagePipe(nullptr, &h0_, &h1_);
    MOJO_ALLOW_UNUSED_LOCAL(result);
    assert(result == MOJO_RESULT_OK);
    result = MojoClose(h0_);
    assert(result == MOJO_RESULT_OK);
    result = MojoClose(h1_);
    assert(result == MOJO_RESULT_OK);
  }

  void MessagePipe_WriteAndRead(void* buffer, uint32_t num_bytes) {
    MojoResult result = MojoWriteMessage(h0_, buffer, num_bytes, nullptr, 0,
                                         MOJO_WRITE_MESSAGE_FLAG_NONE);
    MOJO_ALLOW_UNUSED_LOCAL(result);
    assert(result == MOJO_RESULT_OK);
    uint32_t read_bytes = num_bytes;
    result = MojoReadMessage(h1_, buffer, &read_bytes, nullptr, nullptr,
                             MOJO_READ_MESSAGE_FLAG_NONE);
    assert(result == MOJO_RESULT_OK);
  }

  void MessagePipe_EmptyRead() {
    MojoResult result = MojoReadMessage(h0_, nullptr, nullptr, nullptr, nullptr,
                                        MOJO_READ_MESSAGE_FLAG_MAY_DISCARD);
    MOJO_ALLOW_UNUSED_LOCAL(result);
    assert(result == MOJO_RESULT_SHOULD_WAIT);
  }

 protected:
#if !defined(WIN32)
  void DoMessagePipeThreadedTest(unsigned num_writers,
                                 unsigned num_readers,
                                 uint32_t num_bytes) {
    static const int64_t kPerftestTimeMicroseconds = 3 * 1000000;

    assert(num_writers > 0u);
    assert(num_readers > 0u);

    MojoResult result = MojoCreateMessagePipe(nullptr, &h0_, &h1_);
    MOJO_ALLOW_UNUSED_LOCAL(result);
    assert(result == MOJO_RESULT_OK);

    std::vector<MessagePipeWriterThread*> writers;
    for (unsigned i = 0u; i < num_writers; i++)
      writers.push_back(new MessagePipeWriterThread(h0_, num_bytes));

    std::vector<MessagePipeReaderThread*> readers;
    for (unsigned i = 0u; i < num_readers; i++)
      readers.push_back(new MessagePipeReaderThread(h1_));

    // Start time here, just before we fire off the threads.
    const MojoTimeTicks start_time = MojoGetTimeTicksNow();

    // Interleave the starts.
    for (unsigned i = 0u; i < num_writers || i < num_readers; i++) {
      if (i < num_writers)
        writers[i]->Start();
      if (i < num_readers)
        readers[i]->Start();
    }

    Sleep(kPerftestTimeMicroseconds);

    // Close both handles to make writers and readers stop immediately.
    result = MojoClose(h0_);
    assert(result == MOJO_RESULT_OK);
    result = MojoClose(h1_);
    assert(result == MOJO_RESULT_OK);

    // Join everything.
    for (unsigned i = 0u; i < num_writers; i++)
      writers[i]->Join();
    for (unsigned i = 0u; i < num_readers; i++)
      readers[i]->Join();

    // Stop time here.
    MojoTimeTicks end_time = MojoGetTimeTicksNow();

    // Add up write and read counts, and destroy the threads.
    int64_t num_writes = 0;
    for (unsigned i = 0u; i < num_writers; i++) {
      num_writes += writers[i]->num_writes();
      delete writers[i];
    }
    writers.clear();
    int64_t num_reads = 0;
    for (unsigned i = 0u; i < num_readers; i++) {
      num_reads += readers[i]->num_reads();
      delete readers[i];
    }
    readers.clear();

    char sub_test_name[200];
    sprintf(sub_test_name, "%uw_%ur_%ubytes", num_writers, num_readers,
            static_cast<unsigned>(num_bytes));
    mojo::test::LogPerfResult(
        "MessagePipe_Threaded_Writes", sub_test_name,
        1000000.0 * static_cast<double>(num_writes) / (end_time - start_time),
        "writes/second");
    mojo::test::LogPerfResult(
        "MessagePipe_Threaded_Reads", sub_test_name,
        1000000.0 * static_cast<double>(num_reads) / (end_time - start_time),
        "reads/second");
  }
#endif  // !defined(WIN32)

  MojoHandle h0_;
  MojoHandle h1_;

 private:
#if !defined(WIN32)
  void Sleep(int64_t microseconds) {
    struct timespec req = {
        static_cast<time_t>(microseconds / 1000000),       // Seconds.
        static_cast<long>(microseconds % 1000000) * 1000L  // Nanoseconds.
    };
    int rv = nanosleep(&req, nullptr);
    MOJO_ALLOW_UNUSED_LOCAL(rv);
    assert(rv == 0);
  }
#endif  // !defined(WIN32)

  MOJO_DISALLOW_COPY_AND_ASSIGN(CorePerftest);
};

// A no-op test so we can compare performance.
TEST_F(CorePerftest, NoOp) {
  IterateAndReportPerf("Iterate_NoOp", nullptr, [this]() { NoOp(); });
}

TEST_F(CorePerftest, MessagePipe_CreateAndClose) {
  IterateAndReportPerf("MessagePipe_CreateAndClose", nullptr,
                       [this]() { MessagePipe_CreateAndClose(); });
}

TEST_F(CorePerftest, MessagePipe_WriteAndRead) {
  MojoResult result = MojoCreateMessagePipe(nullptr, &h0_, &h1_);
  MOJO_ALLOW_UNUSED_LOCAL(result);
  assert(result == MOJO_RESULT_OK);
  char buffer[10000] = {};
  IterateAndReportPerf(
      "MessagePipe_WriteAndRead", "10bytes",
      [this, &buffer]() { MessagePipe_WriteAndRead(buffer, 10u); });
  IterateAndReportPerf(
      "MessagePipe_WriteAndRead", "100bytes",
      [this, &buffer]() { MessagePipe_WriteAndRead(buffer, 100u); });
  IterateAndReportPerf(
      "MessagePipe_WriteAndRead", "1000bytes",
      [this, &buffer]() { MessagePipe_WriteAndRead(buffer, 1000u); });
  IterateAndReportPerf(
      "MessagePipe_WriteAndRead", "10000bytes",
      [this, &buffer]() { MessagePipe_WriteAndRead(buffer, 10000u); });
  result = MojoClose(h0_);
  assert(result == MOJO_RESULT_OK);
  result = MojoClose(h1_);
  assert(result == MOJO_RESULT_OK);
}

TEST_F(CorePerftest, MessagePipe_EmptyRead) {
  MojoResult result = MojoCreateMessagePipe(nullptr, &h0_, &h1_);
  MOJO_ALLOW_UNUSED_LOCAL(result);
  assert(result == MOJO_RESULT_OK);
  IterateAndReportPerf("MessagePipe_EmptyRead", nullptr,
                       [this]() { MessagePipe_EmptyRead(); });
  result = MojoClose(h0_);
  assert(result == MOJO_RESULT_OK);
  result = MojoClose(h1_);
  assert(result == MOJO_RESULT_OK);
}

#if !defined(WIN32)
TEST_F(CorePerftest, MessagePipe_Threaded) {
  DoMessagePipeThreadedTest(1u, 1u, 100u);
  DoMessagePipeThreadedTest(2u, 2u, 100u);
  DoMessagePipeThreadedTest(3u, 3u, 100u);
  DoMessagePipeThreadedTest(10u, 10u, 100u);
  DoMessagePipeThreadedTest(10u, 1u, 100u);
  DoMessagePipeThreadedTest(1u, 10u, 100u);

  // For comparison of overhead:
  DoMessagePipeThreadedTest(1u, 1u, 10u);
  // 100 was done above.
  DoMessagePipeThreadedTest(1u, 1u, 1000u);
  DoMessagePipeThreadedTest(1u, 1u, 10000u);

  DoMessagePipeThreadedTest(3u, 3u, 10u);
  // 100 was done above.
  DoMessagePipeThreadedTest(3u, 3u, 1000u);
  DoMessagePipeThreadedTest(3u, 3u, 10000u);
}
#endif  // !defined(WIN32)

}  // namespace
