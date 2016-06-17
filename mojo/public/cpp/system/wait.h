// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_SYSTEM_WAIT_H_
#define MOJO_PUBLIC_CPP_SYSTEM_WAIT_H_

#include <stdint.h>

#include <cstddef>

#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/result.h"
#include "mojo/public/c/system/time.h"
#include "mojo/public/c/system/wait.h"
#include "mojo/public/cpp/system/handle.h"

namespace mojo {

inline MojoResult Wait(Handle handle,
                       MojoHandleSignals signals,
                       MojoDeadline deadline,
                       MojoHandleSignalsState* signals_state) {
  return MojoWait(handle.value(), signals, deadline, signals_state);
}

const uint32_t kInvalidWaitManyIndexValue = static_cast<uint32_t>(-1);

// Simplify the interpretation of the output from |MojoWaitMany()|.
struct WaitManyResult {
  explicit WaitManyResult(MojoResult mojo_wait_many_result)
      : result(mojo_wait_many_result), index(kInvalidWaitManyIndexValue) {}

  WaitManyResult(MojoResult mojo_wait_many_result, uint32_t result_index)
      : result(mojo_wait_many_result), index(result_index) {}

  // A valid handle index is always returned if |WaitMany()| succeeds, but may
  // or may not be returned if |WaitMany()| returns an error. Use this helper
  // function to check if |index| is a valid index into the handle array.
  bool IsIndexValid() const { return index != kInvalidWaitManyIndexValue; }

  // The |signals_states| array is always returned by |WaitMany()| on success,
  // but may or may not be returned if |WaitMany()| returns an error. Use this
  // helper function to check if |signals_states| holds valid data.
  bool AreSignalsStatesValid() const {
    return result != MOJO_RESULT_INVALID_ARGUMENT &&
           result != MOJO_RESULT_RESOURCE_EXHAUSTED &&
           result != MOJO_RESULT_BUSY;
  }

  MojoResult result;
  uint32_t index;
};

// |HandleVectorType| and |FlagsVectorType| should be similar enough to
// |std::vector<Handle>| and |std::vector<MojoHandleSignals>|, respectively:
//  - They should have a (const) |size()| method that returns an unsigned type.
//  - They must provide contiguous storage, with access via (const) reference to
//    that storage provided by a (const) |operator[]()| (by reference).
template <class HandleVectorType,
          class FlagsVectorType,
          class SignalsStateVectorType>
inline WaitManyResult WaitMany(const HandleVectorType& handles,
                               const FlagsVectorType& signals,
                               MojoDeadline deadline,
                               SignalsStateVectorType* signals_states) {
  if (signals.size() != handles.size() ||
      (signals_states && signals_states->size() != signals.size()))
    return WaitManyResult(MOJO_RESULT_INVALID_ARGUMENT);
  if (handles.size() >= kInvalidWaitManyIndexValue)
    return WaitManyResult(MOJO_RESULT_RESOURCE_EXHAUSTED);

  if (handles.size() == 0) {
    return WaitManyResult(
        MojoWaitMany(nullptr, nullptr, 0, deadline, nullptr, nullptr));
  }

  uint32_t result_index = kInvalidWaitManyIndexValue;
  const Handle& first_handle = handles[0];
  const MojoHandleSignals& first_signals = signals[0];
  MojoHandleSignalsState* first_state =
      signals_states ? &(*signals_states)[0] : nullptr;
  MojoResult result =
      MojoWaitMany(reinterpret_cast<const MojoHandle*>(&first_handle),
                   &first_signals, static_cast<uint32_t>(handles.size()),
                   deadline, &result_index, first_state);
  return WaitManyResult(result, result_index);
}

// C++ 4.10, regarding pointer conversion, says that an integral null pointer
// constant can be converted to |std::nullptr_t|. The opposite direction is not
// allowed.
template <class HandleVectorType, class FlagsVectorType>
inline WaitManyResult WaitMany(const HandleVectorType& handles,
                               const FlagsVectorType& signals,
                               MojoDeadline deadline,
                               std::nullptr_t signals_states) {
  if (signals.size() != handles.size())
    return WaitManyResult(MOJO_RESULT_INVALID_ARGUMENT);
  if (handles.size() >= kInvalidWaitManyIndexValue)
    return WaitManyResult(MOJO_RESULT_RESOURCE_EXHAUSTED);

  if (handles.size() == 0) {
    return WaitManyResult(
        MojoWaitMany(nullptr, nullptr, 0, deadline, nullptr, nullptr));
  }

  uint32_t result_index = kInvalidWaitManyIndexValue;
  const Handle& first_handle = handles[0];
  const MojoHandleSignals& first_signals = signals[0];
  MojoResult result = MojoWaitMany(
      reinterpret_cast<const MojoHandle*>(&first_handle), &first_signals,
      static_cast<uint32_t>(handles.size()), deadline, &result_index, nullptr);
  return WaitManyResult(result, result_index);
}

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_SYSTEM_WAIT_H_
