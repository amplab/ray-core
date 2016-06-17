// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file tests the C++ Mojo system core wrappers.
// TODO(vtl): Maybe rename "CoreCppTest" -> "CoreTest" if/when this gets
// compiled into a different binary from the C API tests.

#include <stddef.h>

#include <map>

#include "mojo/public/cpp/system/buffer.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/handle.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "mojo/public/cpp/system/time.h"
#include "mojo/public/cpp/system/wait.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace {

TEST(CoreCppTest, GetTimeTicksNow) {
  const MojoTimeTicks start = GetTimeTicksNow();
  EXPECT_NE(static_cast<MojoTimeTicks>(0), start)
      << "GetTimeTicksNow should return nonzero value";
}

TEST(CoreCppTest, Basic) {
  // Basic |Handle| implementation:
  {
    EXPECT_EQ(MOJO_HANDLE_INVALID, kInvalidHandleValue);

    Handle h0;
    EXPECT_EQ(kInvalidHandleValue, h0.value());
    EXPECT_EQ(kInvalidHandleValue, *h0.mutable_value());
    EXPECT_FALSE(h0.is_valid());

    Handle h1(static_cast<MojoHandle>(123));
    EXPECT_EQ(static_cast<MojoHandle>(123), h1.value());
    EXPECT_EQ(static_cast<MojoHandle>(123), *h1.mutable_value());
    EXPECT_TRUE(h1.is_valid());
    *h1.mutable_value() = static_cast<MojoHandle>(456);
    EXPECT_EQ(static_cast<MojoHandle>(456), h1.value());
    EXPECT_TRUE(h1.is_valid());

    h1.swap(h0);
    EXPECT_EQ(static_cast<MojoHandle>(456), h0.value());
    EXPECT_TRUE(h0.is_valid());
    EXPECT_FALSE(h1.is_valid());

    h1.set_value(static_cast<MojoHandle>(789));
    h0.swap(h1);
    EXPECT_EQ(static_cast<MojoHandle>(789), h0.value());
    EXPECT_TRUE(h0.is_valid());
    EXPECT_EQ(static_cast<MojoHandle>(456), h1.value());
    EXPECT_TRUE(h1.is_valid());

    // Make sure copy constructor works.
    Handle h2(h0);
    EXPECT_EQ(static_cast<MojoHandle>(789), h2.value());
    // And assignment.
    h2 = h1;
    EXPECT_EQ(static_cast<MojoHandle>(456), h2.value());

    // Make sure that we can put |Handle|s into |std::map|s.
    h0 = Handle(static_cast<MojoHandle>(987));
    h1 = Handle(static_cast<MojoHandle>(654));
    h2 = Handle(static_cast<MojoHandle>(321));
    Handle h3;
    std::map<Handle, int> handle_to_int;
    handle_to_int[h0] = 0;
    handle_to_int[h1] = 1;
    handle_to_int[h2] = 2;
    handle_to_int[h3] = 3;

    EXPECT_EQ(4u, handle_to_int.size());
    EXPECT_FALSE(handle_to_int.find(h0) == handle_to_int.end());
    EXPECT_EQ(0, handle_to_int[h0]);
    EXPECT_FALSE(handle_to_int.find(h1) == handle_to_int.end());
    EXPECT_EQ(1, handle_to_int[h1]);
    EXPECT_FALSE(handle_to_int.find(h2) == handle_to_int.end());
    EXPECT_EQ(2, handle_to_int[h2]);
    EXPECT_FALSE(handle_to_int.find(h3) == handle_to_int.end());
    EXPECT_EQ(3, handle_to_int[h3]);
    EXPECT_TRUE(handle_to_int.find(Handle(static_cast<MojoHandle>(13579))) ==
                handle_to_int.end());

    // TODO(vtl): With C++11, support |std::unordered_map|s, etc. (Or figure out
    // how to support the variations of |hash_map|.)
  }

  // |Handle|/|ScopedHandle| functions:
  {
    ScopedHandle h;

    EXPECT_EQ(kInvalidHandleValue, h.get().value());

    // This should be a no-op.
    Close(h.Pass());

    // It should still be invalid.
    EXPECT_EQ(kInvalidHandleValue, h.get().value());

    EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
              Wait(h.get(), ~MOJO_HANDLE_SIGNAL_NONE, 1000000, nullptr));

    std::vector<Handle> wh;
    wh.push_back(h.get());
    std::vector<MojoHandleSignals> sigs;
    sigs.push_back(~MOJO_HANDLE_SIGNAL_NONE);
    WaitManyResult wait_many_result =
        WaitMany(wh, sigs, MOJO_DEADLINE_INDEFINITE, nullptr);
    EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, wait_many_result.result);
    EXPECT_TRUE(wait_many_result.IsIndexValid());
    EXPECT_FALSE(wait_many_result.AreSignalsStatesValid());

    // Make sure that our specialized template correctly handles |NULL| as well
    // as |nullptr|.
    wait_many_result = WaitMany(wh, sigs, MOJO_DEADLINE_INDEFINITE, NULL);
    EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, wait_many_result.result);
    EXPECT_EQ(0u, wait_many_result.index);
    EXPECT_TRUE(wait_many_result.IsIndexValid());
    EXPECT_FALSE(wait_many_result.AreSignalsStatesValid());
  }

  // |MakeScopedHandle| (just compilation tests):
  {
    EXPECT_FALSE(MakeScopedHandle(Handle()).is_valid());
    EXPECT_FALSE(MakeScopedHandle(MessagePipeHandle()).is_valid());
    EXPECT_FALSE(MakeScopedHandle(DataPipeProducerHandle()).is_valid());
    EXPECT_FALSE(MakeScopedHandle(DataPipeConsumerHandle()).is_valid());
    EXPECT_FALSE(MakeScopedHandle(SharedBufferHandle()).is_valid());
  }

  // |MessagePipeHandle|/|ScopedMessagePipeHandle| functions:
  {
    MessagePipeHandle h_invalid;
    EXPECT_FALSE(h_invalid.is_valid());
    EXPECT_EQ(
        MOJO_RESULT_INVALID_ARGUMENT,
        WriteMessageRaw(
            h_invalid, nullptr, 0, nullptr, 0, MOJO_WRITE_MESSAGE_FLAG_NONE));
    char buffer[10] = {0};
    EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
              WriteMessageRaw(h_invalid,
                              buffer,
                              sizeof(buffer),
                              nullptr,
                              0,
                              MOJO_WRITE_MESSAGE_FLAG_NONE));
    EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
              ReadMessageRaw(h_invalid,
                             nullptr,
                             nullptr,
                             nullptr,
                             nullptr,
                             MOJO_READ_MESSAGE_FLAG_NONE));
    uint32_t buffer_size = static_cast<uint32_t>(sizeof(buffer));
    EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
              ReadMessageRaw(h_invalid,
                             buffer,
                             &buffer_size,
                             nullptr,
                             nullptr,
                             MOJO_READ_MESSAGE_FLAG_NONE));

    // Basic tests of waiting and closing.
    MojoHandle hv0 = kInvalidHandleValue;
    {
      ScopedMessagePipeHandle h0;
      ScopedMessagePipeHandle h1;
      EXPECT_FALSE(h0.get().is_valid());
      EXPECT_FALSE(h1.get().is_valid());

      ASSERT_EQ(MOJO_RESULT_OK, CreateMessagePipe(nullptr, &h0, &h1));
      EXPECT_TRUE(h0.get().is_valid());
      EXPECT_TRUE(h1.get().is_valid());
      EXPECT_NE(h0.get().value(), h1.get().value());
      // Save the handle values, so we can check that things got closed
      // correctly.
      hv0 = h0.get().value();
      MojoHandle hv1 = h1.get().value();
      MojoHandleSignalsState state;

      EXPECT_EQ(MOJO_RESULT_DEADLINE_EXCEEDED,
                Wait(h0.get(), MOJO_HANDLE_SIGNAL_READABLE, 0, &state));
      EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE, state.satisfied_signals);
      EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE |
                    MOJO_HANDLE_SIGNAL_PEER_CLOSED,
                state.satisfiable_signals);

      std::vector<Handle> wh;
      wh.push_back(h0.get());
      wh.push_back(h1.get());
      std::vector<MojoHandleSignals> sigs;
      sigs.push_back(MOJO_HANDLE_SIGNAL_READABLE);
      sigs.push_back(MOJO_HANDLE_SIGNAL_WRITABLE);
      std::vector<MojoHandleSignalsState> states(sigs.size());
      WaitManyResult wait_many_result = WaitMany(wh, sigs, 1000, &states);
      EXPECT_EQ(MOJO_RESULT_OK, wait_many_result.result);
      EXPECT_EQ(1u, wait_many_result.index);
      EXPECT_TRUE(wait_many_result.IsIndexValid());
      EXPECT_TRUE(wait_many_result.AreSignalsStatesValid());
      EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE, states[0].satisfied_signals);
      EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE |
                    MOJO_HANDLE_SIGNAL_PEER_CLOSED,
                states[0].satisfiable_signals);
      EXPECT_EQ(MOJO_HANDLE_SIGNAL_WRITABLE, states[1].satisfied_signals);
      EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE |
                    MOJO_HANDLE_SIGNAL_PEER_CLOSED,
                states[1].satisfiable_signals);

      // Test closing |h1| explicitly.
      Close(h1.Pass());
      EXPECT_FALSE(h1.get().is_valid());

      // Make sure |h1| is closed.
      EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT,
                Wait(Handle(hv1), ~MOJO_HANDLE_SIGNAL_NONE,
                     MOJO_DEADLINE_INDEFINITE, nullptr));

      EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION,
                Wait(h0.get(), MOJO_HANDLE_SIGNAL_READABLE,
                     MOJO_DEADLINE_INDEFINITE, &state));
      EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, state.satisfied_signals);
      EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, state.satisfiable_signals);
    }
    // |hv0| should have been closed when |h0| went out of scope, so this close
    // should fail.
    EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoClose(hv0));

    // Actually test writing/reading messages.
    {
      ScopedMessagePipeHandle h0;
      ScopedMessagePipeHandle h1;
      ASSERT_EQ(MOJO_RESULT_OK, CreateMessagePipe(nullptr, &h0, &h1));

      const char kHello[] = "hello";
      const uint32_t kHelloSize = static_cast<uint32_t>(sizeof(kHello));
      EXPECT_EQ(MOJO_RESULT_OK,
                WriteMessageRaw(h0.get(),
                                kHello,
                                kHelloSize,
                                nullptr,
                                0,
                                MOJO_WRITE_MESSAGE_FLAG_NONE));

      MojoHandleSignalsState state;
      EXPECT_EQ(MOJO_RESULT_OK, Wait(h1.get(), MOJO_HANDLE_SIGNAL_READABLE,
                                     MOJO_DEADLINE_INDEFINITE, &state));
      EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE,
                state.satisfied_signals);
      EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE |
                    MOJO_HANDLE_SIGNAL_PEER_CLOSED,
                state.satisfiable_signals);

      char buffer[10] = {0};
      uint32_t buffer_size = static_cast<uint32_t>(sizeof(buffer));
      EXPECT_EQ(MOJO_RESULT_OK,
                ReadMessageRaw(h1.get(),
                               buffer,
                               &buffer_size,
                               nullptr,
                               nullptr,
                               MOJO_READ_MESSAGE_FLAG_NONE));
      EXPECT_EQ(kHelloSize, buffer_size);
      EXPECT_STREQ(kHello, buffer);

      // Send a handle over the previously-establish message pipe. Use the
      // |MessagePipe| wrapper (to test it), which automatically creates a
      // message pipe.
      MessagePipe mp;

      // Write a message to |mp.handle0|, before we send |mp.handle1|.
      const char kWorld[] = "world!";
      const uint32_t kWorldSize = static_cast<uint32_t>(sizeof(kWorld));
      EXPECT_EQ(MOJO_RESULT_OK,
                WriteMessageRaw(mp.handle0.get(),
                                kWorld,
                                kWorldSize,
                                nullptr,
                                0,
                                MOJO_WRITE_MESSAGE_FLAG_NONE));

      // Send |mp.handle1| over |h1| to |h0|.
      MojoHandle handles[5];
      handles[0] = mp.handle1.release().value();
      EXPECT_NE(kInvalidHandleValue, handles[0]);
      EXPECT_FALSE(mp.handle1.get().is_valid());
      uint32_t handles_count = 1;
      EXPECT_EQ(MOJO_RESULT_OK,
                WriteMessageRaw(h1.get(),
                                kHello,
                                kHelloSize,
                                handles,
                                handles_count,
                                MOJO_WRITE_MESSAGE_FLAG_NONE));
      // |handles[0]| should actually be invalid now.
      EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoClose(handles[0]));

      // Read "hello" and the sent handle.
      EXPECT_EQ(MOJO_RESULT_OK, Wait(h0.get(), MOJO_HANDLE_SIGNAL_READABLE,
                                     MOJO_DEADLINE_INDEFINITE, &state));
      EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE,
                state.satisfied_signals);
      EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE |
                    MOJO_HANDLE_SIGNAL_PEER_CLOSED,
                state.satisfiable_signals);

      memset(buffer, 0, sizeof(buffer));
      buffer_size = static_cast<uint32_t>(sizeof(buffer));
      for (size_t i = 0; i < MOJO_ARRAYSIZE(handles); i++)
        handles[i] = kInvalidHandleValue;
      handles_count = static_cast<uint32_t>(MOJO_ARRAYSIZE(handles));
      EXPECT_EQ(MOJO_RESULT_OK,
                ReadMessageRaw(h0.get(),
                               buffer,
                               &buffer_size,
                               handles,
                               &handles_count,
                               MOJO_READ_MESSAGE_FLAG_NONE));
      EXPECT_EQ(kHelloSize, buffer_size);
      EXPECT_STREQ(kHello, buffer);
      EXPECT_EQ(1u, handles_count);
      EXPECT_NE(kInvalidHandleValue, handles[0]);

      // Read from the sent/received handle.
      mp.handle1.reset(MessagePipeHandle(handles[0]));
      // Save |handles[0]| to check that it gets properly closed.
      hv0 = handles[0];

      EXPECT_EQ(MOJO_RESULT_OK,
                Wait(mp.handle1.get(), MOJO_HANDLE_SIGNAL_READABLE,
                     MOJO_DEADLINE_INDEFINITE, &state));
      EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE,
                state.satisfied_signals);
      EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_WRITABLE |
                    MOJO_HANDLE_SIGNAL_PEER_CLOSED,
                state.satisfiable_signals);

      memset(buffer, 0, sizeof(buffer));
      buffer_size = static_cast<uint32_t>(sizeof(buffer));
      for (size_t i = 0; i < MOJO_ARRAYSIZE(handles); i++)
        handles[i] = kInvalidHandleValue;
      handles_count = static_cast<uint32_t>(MOJO_ARRAYSIZE(handles));
      EXPECT_EQ(MOJO_RESULT_OK,
                ReadMessageRaw(mp.handle1.get(),
                               buffer,
                               &buffer_size,
                               handles,
                               &handles_count,
                               MOJO_READ_MESSAGE_FLAG_NONE));
      EXPECT_EQ(kWorldSize, buffer_size);
      EXPECT_STREQ(kWorld, buffer);
      EXPECT_EQ(0u, handles_count);
    }
    EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoClose(hv0));
  }

  // TODO(vtl): Test |CloseRaw()|.
  // TODO(vtl): Test |reset()| more thoroughly?
}

TEST(CoreCppTest, TearDownWithMessagesEnqueued) {
  // Tear down a message pipe which still has a message enqueued, with the
  // message also having a valid message pipe handle.
  {
    ScopedMessagePipeHandle h0;
    ScopedMessagePipeHandle h1;
    ASSERT_EQ(MOJO_RESULT_OK, CreateMessagePipe(nullptr, &h0, &h1));

    // Send a handle over the previously-establish message pipe.
    ScopedMessagePipeHandle h2;
    ScopedMessagePipeHandle h3;
    ASSERT_EQ(MOJO_RESULT_OK, CreateMessagePipe(nullptr, &h2, &h3));

    // Write a message to |h2|, before we send |h3|.
    const char kWorld[] = "world!";
    const uint32_t kWorldSize = static_cast<uint32_t>(sizeof(kWorld));
    EXPECT_EQ(MOJO_RESULT_OK,
              WriteMessageRaw(h2.get(),
                              kWorld,
                              kWorldSize,
                              nullptr,
                              0,
                              MOJO_WRITE_MESSAGE_FLAG_NONE));
    // And also a message to |h3|.
    EXPECT_EQ(MOJO_RESULT_OK,
              WriteMessageRaw(h3.get(),
                              kWorld,
                              kWorldSize,
                              nullptr,
                              0,
                              MOJO_WRITE_MESSAGE_FLAG_NONE));

    // Send |h3| over |h1| to |h0|.
    const char kHello[] = "hello";
    const uint32_t kHelloSize = static_cast<uint32_t>(sizeof(kHello));
    MojoHandle h3_value;
    h3_value = h3.release().value();
    EXPECT_NE(kInvalidHandleValue, h3_value);
    EXPECT_FALSE(h3.get().is_valid());
    EXPECT_EQ(MOJO_RESULT_OK,
              WriteMessageRaw(h1.get(),
                              kHello,
                              kHelloSize,
                              &h3_value,
                              1,
                              MOJO_WRITE_MESSAGE_FLAG_NONE));
    // |h3_value| should actually be invalid now.
    EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoClose(h3_value));

    EXPECT_EQ(MOJO_RESULT_OK, MojoClose(h0.release().value()));
    EXPECT_EQ(MOJO_RESULT_OK, MojoClose(h1.release().value()));
    EXPECT_EQ(MOJO_RESULT_OK, MojoClose(h2.release().value()));
  }

  // Do this in a different order: make the enqueued message pipe handle only
  // half-alive.
  {
    ScopedMessagePipeHandle h0;
    ScopedMessagePipeHandle h1;
    ASSERT_EQ(MOJO_RESULT_OK, CreateMessagePipe(nullptr, &h0, &h1));

    // Send a handle over the previously-establish message pipe.
    ScopedMessagePipeHandle h2;
    ScopedMessagePipeHandle h3;
    ASSERT_EQ(MOJO_RESULT_OK, CreateMessagePipe(nullptr, &h2, &h3));

    // Write a message to |h2|, before we send |h3|.
    const char kWorld[] = "world!";
    const uint32_t kWorldSize = static_cast<uint32_t>(sizeof(kWorld));
    EXPECT_EQ(MOJO_RESULT_OK,
              WriteMessageRaw(h2.get(),
                              kWorld,
                              kWorldSize,
                              nullptr,
                              0,
                              MOJO_WRITE_MESSAGE_FLAG_NONE));
    // And also a message to |h3|.
    EXPECT_EQ(MOJO_RESULT_OK,
              WriteMessageRaw(h3.get(),
                              kWorld,
                              kWorldSize,
                              nullptr,
                              0,
                              MOJO_WRITE_MESSAGE_FLAG_NONE));

    // Send |h3| over |h1| to |h0|.
    const char kHello[] = "hello";
    const uint32_t kHelloSize = static_cast<uint32_t>(sizeof(kHello));
    MojoHandle h3_value;
    h3_value = h3.release().value();
    EXPECT_NE(kInvalidHandleValue, h3_value);
    EXPECT_FALSE(h3.get().is_valid());
    EXPECT_EQ(MOJO_RESULT_OK,
              WriteMessageRaw(h1.get(),
                              kHello,
                              kHelloSize,
                              &h3_value,
                              1,
                              MOJO_WRITE_MESSAGE_FLAG_NONE));
    // |h3_value| should actually be invalid now.
    EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, MojoClose(h3_value));

    EXPECT_EQ(MOJO_RESULT_OK, MojoClose(h2.release().value()));
    EXPECT_EQ(MOJO_RESULT_OK, MojoClose(h0.release().value()));
    EXPECT_EQ(MOJO_RESULT_OK, MojoClose(h1.release().value()));
  }
}

TEST(CoreCppTest, ScopedHandleMoveCtor) {
  ScopedSharedBufferHandle buffer1;
  EXPECT_EQ(MOJO_RESULT_OK, CreateSharedBuffer(nullptr, 1024, &buffer1));
  EXPECT_TRUE(buffer1.is_valid());

  ScopedSharedBufferHandle buffer2;
  EXPECT_EQ(MOJO_RESULT_OK, CreateSharedBuffer(nullptr, 1024, &buffer2));
  EXPECT_TRUE(buffer2.is_valid());

  // If this fails to close buffer1, ScopedHandleBase::CloseIfNecessary() will
  // assert.
  buffer1 = buffer2.Pass();

  EXPECT_TRUE(buffer1.is_valid());
  EXPECT_FALSE(buffer2.is_valid());
}

TEST(CoreCppTest, ScopedHandleMoveCtorSelf) {
  ScopedSharedBufferHandle buffer1;
  EXPECT_EQ(MOJO_RESULT_OK, CreateSharedBuffer(nullptr, 1024, &buffer1));
  EXPECT_TRUE(buffer1.is_valid());

  buffer1 = buffer1.Pass();

  EXPECT_TRUE(buffer1.is_valid());
}

TEST(CoreCppTest, WaitManyResult) {
  {
    WaitManyResult wmr(MOJO_RESULT_OK);
    EXPECT_FALSE(wmr.IsIndexValid());
    EXPECT_TRUE(wmr.AreSignalsStatesValid());
    EXPECT_EQ(MOJO_RESULT_OK, wmr.result);
  }

  {
    WaitManyResult wmr(MOJO_RESULT_FAILED_PRECONDITION);
    EXPECT_FALSE(wmr.IsIndexValid());
    EXPECT_TRUE(wmr.AreSignalsStatesValid());
    EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION, wmr.result);
  }

  {
    WaitManyResult wmr(MOJO_RESULT_INVALID_ARGUMENT);
    EXPECT_FALSE(wmr.IsIndexValid());
    EXPECT_FALSE(wmr.AreSignalsStatesValid());
    EXPECT_EQ(MOJO_RESULT_INVALID_ARGUMENT, wmr.result);
  }

  // These should be like "invalid argument".
  EXPECT_FALSE(
      WaitManyResult(MOJO_RESULT_RESOURCE_EXHAUSTED).AreSignalsStatesValid());
  EXPECT_FALSE(WaitManyResult(MOJO_RESULT_BUSY).AreSignalsStatesValid());

  {
    WaitManyResult wmr(MOJO_RESULT_OK, 5u);
    EXPECT_TRUE(wmr.IsIndexValid());
    EXPECT_TRUE(wmr.AreSignalsStatesValid());
    EXPECT_EQ(MOJO_RESULT_OK, wmr.result);
    EXPECT_EQ(5u, wmr.index);
  }

  {
    WaitManyResult wmr(MOJO_RESULT_FAILED_PRECONDITION, 5u);
    EXPECT_TRUE(wmr.IsIndexValid());
    EXPECT_TRUE(wmr.AreSignalsStatesValid());
    EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION, wmr.result);
    EXPECT_EQ(5u, wmr.index);
  }
}

// TODO(ncbray): enable this test once NaCl supports the corresponding APIs.
#ifdef __native_client__
#define MAYBE_DataPipe DISABLED_DataPipe
#else
#define MAYBE_DataPipe DataPipe
#endif
TEST(CoreCppTest, MAYBE_DataPipe) {
  ScopedDataPipeProducerHandle ph;
  ScopedDataPipeConsumerHandle ch;

  ASSERT_EQ(MOJO_RESULT_OK, CreateDataPipe(nullptr, &ph, &ch));
  ASSERT_TRUE(ph.get().is_valid());
  ASSERT_TRUE(ch.get().is_valid());

  uint32_t read_threshold = 123u;
  EXPECT_EQ(MOJO_RESULT_OK,
            GetDataPipeConsumerOptions(ch.get(), &read_threshold));
  EXPECT_EQ(0u, read_threshold);

  EXPECT_EQ(MOJO_RESULT_OK, SetDataPipeConsumerOptions(ch.get(), 2u));

  EXPECT_EQ(MOJO_RESULT_OK,
            GetDataPipeConsumerOptions(ch.get(), &read_threshold));
  EXPECT_EQ(2u, read_threshold);

  // Write a byte.
  static const char kA = 'A';
  uint32_t num_bytes = 1u;
  EXPECT_EQ(MOJO_RESULT_OK,
            WriteDataRaw(ph.get(), &kA, &num_bytes, MOJO_WRITE_DATA_FLAG_NONE));

  // Waiting for "read threshold" should fail. (Wait a nonzero amount, in case
  // there's some latency.)
  MojoHandleSignalsState state;
  EXPECT_EQ(MOJO_RESULT_DEADLINE_EXCEEDED,
            Wait(ch.get(), MOJO_HANDLE_SIGNAL_READ_THRESHOLD, 1000, &state));
  // ... but it should be readable.
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE, state.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            state.satisfiable_signals);

  // Do a two-phase write of another byte.
  void* write_buffer = nullptr;
  num_bytes = 0u;
  ASSERT_EQ(MOJO_RESULT_OK,
            BeginWriteDataRaw(ph.get(), &write_buffer, &num_bytes,
                              MOJO_WRITE_DATA_FLAG_NONE));
  ASSERT_TRUE(write_buffer);
  ASSERT_GT(num_bytes, 0u);
  static_cast<char*>(write_buffer)[0] = 'B';
  EXPECT_EQ(MOJO_RESULT_OK, EndWriteDataRaw(ph.get(), 1u));

  // Now waiting for "read threshold" should succeed. (Wait a nonzero amount, in
  // case there's some latency.)
  state = MojoHandleSignalsState();
  EXPECT_EQ(MOJO_RESULT_OK,
            Wait(ch.get(), MOJO_HANDLE_SIGNAL_READ_THRESHOLD, 1000, &state));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            state.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_READABLE | MOJO_HANDLE_SIGNAL_PEER_CLOSED |
                MOJO_HANDLE_SIGNAL_READ_THRESHOLD,
            state.satisfiable_signals);

  // Read a byte.
  char read_byte = 'x';
  num_bytes = 1u;
  EXPECT_EQ(MOJO_RESULT_OK, ReadDataRaw(ch.get(), &read_byte, &num_bytes,
                                        MOJO_READ_DATA_FLAG_NONE));
  EXPECT_EQ(1u, num_bytes);
  EXPECT_EQ('A', read_byte);

  // Waiting for "read threshold" should now fail.
  EXPECT_EQ(MOJO_RESULT_DEADLINE_EXCEEDED,
            Wait(ch.get(), MOJO_HANDLE_SIGNAL_READ_THRESHOLD, 0, nullptr));

  // Reset the read threshold/options.
  EXPECT_EQ(MOJO_RESULT_OK, SetDataPipeConsumerOptionsToDefault(ch.get()));

  // Waiting for "read threshold" should now succeed.
  EXPECT_EQ(MOJO_RESULT_OK,
            Wait(ch.get(), MOJO_HANDLE_SIGNAL_READ_THRESHOLD, 0, nullptr));

  // Do a two-phase read.
  const void* read_buffer = nullptr;
  num_bytes = 0u;
  ASSERT_EQ(MOJO_RESULT_OK, BeginReadDataRaw(ch.get(), &read_buffer, &num_bytes,
                                             MOJO_READ_DATA_FLAG_NONE));
  ASSERT_TRUE(read_buffer);
  ASSERT_EQ(1u, num_bytes);
  EXPECT_EQ('B', static_cast<const char*>(read_buffer)[0]);
  EXPECT_EQ(MOJO_RESULT_OK, EndReadDataRaw(ch.get(), 1u));

  // Waiting for "read" should now fail (time out).
  EXPECT_EQ(MOJO_RESULT_DEADLINE_EXCEEDED,
            Wait(ch.get(), MOJO_HANDLE_SIGNAL_READ_THRESHOLD, 1000, nullptr));

  // Close the producer.
  ph.reset();

  // Waiting for "read" should now fail.
  EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION,
            Wait(ch.get(), MOJO_HANDLE_SIGNAL_READ_THRESHOLD, 1000, nullptr));
}

}  // namespace
}  // namespace mojo
