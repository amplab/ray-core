// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/platform/native/system_thunks.h"

#include <assert.h>

#include "mojo/public/platform/native/thunk_export.h"

static struct MojoSystemThunks g_thunks = {0};

MojoTimeTicks MojoGetTimeTicksNow() {
  assert(g_thunks.GetTimeTicksNow);
  return g_thunks.GetTimeTicksNow();
}

MojoResult MojoClose(MojoHandle handle) {
  assert(g_thunks.Close);
  return g_thunks.Close(handle);
}

MojoResult MojoGetRights(MojoHandle handle, MojoHandleRights* rights) {
  assert(g_thunks.GetRights);
  return g_thunks.GetRights(handle, rights);
}

MojoResult MojoReplaceHandleWithReducedRights(
    MojoHandle handle,
    MojoHandleRights rights_to_remove,
    MojoHandle* replacement_handle) {
  assert(g_thunks.ReplaceHandleWithReducedRights);
  return g_thunks.ReplaceHandleWithReducedRights(
      handle, rights_to_remove, replacement_handle);
}

MojoResult MojoDuplicateHandleWithReducedRights(
    MojoHandle handle,
    MojoHandleRights rights_to_remove,
    MojoHandle* new_handle) {
  assert(g_thunks.DuplicateHandleWithReducedRights);
  return g_thunks.DuplicateHandleWithReducedRights(handle, rights_to_remove,
                                                   new_handle);
}

MojoResult MojoDuplicateHandle(MojoHandle handle, MojoHandle* new_handle) {
  assert(g_thunks.DuplicateHandle);
  return g_thunks.DuplicateHandle(handle, new_handle);
}

MojoResult MojoWait(MojoHandle handle,
                    MojoHandleSignals signals,
                    MojoDeadline deadline,
                    struct MojoHandleSignalsState* signals_state) {
  assert(g_thunks.Wait);
  return g_thunks.Wait(handle, signals, deadline, signals_state);
}

MojoResult MojoWaitMany(const MojoHandle* handles,
                        const MojoHandleSignals* signals,
                        uint32_t num_handles,
                        MojoDeadline deadline,
                        uint32_t* result_index,
                        struct MojoHandleSignalsState* signals_states) {
  assert(g_thunks.WaitMany);
  return g_thunks.WaitMany(handles, signals, num_handles, deadline,
                           result_index, signals_states);
}

MojoResult MojoCreateMessagePipe(
    const struct MojoCreateMessagePipeOptions* options,
    MojoHandle* message_pipe_handle0,
    MojoHandle* message_pipe_handle1) {
  assert(g_thunks.CreateMessagePipe);
  return g_thunks.CreateMessagePipe(options, message_pipe_handle0,
                                    message_pipe_handle1);
}

MojoResult MojoWriteMessage(MojoHandle message_pipe_handle,
                            const void* bytes,
                            uint32_t num_bytes,
                            const MojoHandle* handles,
                            uint32_t num_handles,
                            MojoWriteMessageFlags flags) {
  assert(g_thunks.WriteMessage);
  return g_thunks.WriteMessage(message_pipe_handle, bytes, num_bytes, handles,
                               num_handles, flags);
}

MojoResult MojoReadMessage(MojoHandle message_pipe_handle,
                           void* bytes,
                           uint32_t* num_bytes,
                           MojoHandle* handles,
                           uint32_t* num_handles,
                           MojoReadMessageFlags flags) {
  assert(g_thunks.ReadMessage);
  return g_thunks.ReadMessage(message_pipe_handle, bytes, num_bytes, handles,
                              num_handles, flags);
}

MojoResult MojoCreateDataPipe(const struct MojoCreateDataPipeOptions* options,
                              MojoHandle* data_pipe_producer_handle,
                              MojoHandle* data_pipe_consumer_handle) {
  assert(g_thunks.CreateDataPipe);
  return g_thunks.CreateDataPipe(options, data_pipe_producer_handle,
                                 data_pipe_consumer_handle);
}

MojoResult MojoSetDataPipeProducerOptions(
    MojoHandle data_pipe_producer_handle,
    const struct MojoDataPipeProducerOptions* options) {
  assert(g_thunks.SetDataPipeProducerOptions);
  return g_thunks.SetDataPipeProducerOptions(data_pipe_producer_handle,
                                             options);
}

MojoResult MojoGetDataPipeProducerOptions(
    MojoHandle data_pipe_producer_handle,
    struct MojoDataPipeProducerOptions* options,
    uint32_t options_num_bytes) {
  assert(g_thunks.GetDataPipeProducerOptions);
  return g_thunks.GetDataPipeProducerOptions(data_pipe_producer_handle,
                                             options, options_num_bytes);
}

MojoResult MojoWriteData(MojoHandle data_pipe_producer_handle,
                         const void* elements,
                         uint32_t* num_elements,
                         MojoWriteDataFlags flags) {
  assert(g_thunks.WriteData);
  return g_thunks.WriteData(data_pipe_producer_handle, elements, num_elements,
                            flags);
}

MojoResult MojoBeginWriteData(MojoHandle data_pipe_producer_handle,
                              void** buffer,
                              uint32_t* buffer_num_elements,
                              MojoWriteDataFlags flags) {
  assert(g_thunks.BeginWriteData);
  return g_thunks.BeginWriteData(data_pipe_producer_handle, buffer,
                                 buffer_num_elements, flags);
}

MojoResult MojoEndWriteData(MojoHandle data_pipe_producer_handle,
                            uint32_t num_elements_written) {
  assert(g_thunks.EndWriteData);
  return g_thunks.EndWriteData(data_pipe_producer_handle, num_elements_written);
}

MojoResult MojoSetDataPipeConsumerOptions(
    MojoHandle data_pipe_consumer_handle,
    const struct MojoDataPipeConsumerOptions* options) {
  assert(g_thunks.SetDataPipeConsumerOptions);
  return g_thunks.SetDataPipeConsumerOptions(data_pipe_consumer_handle,
                                             options);
}

MojoResult MojoGetDataPipeConsumerOptions(
    MojoHandle data_pipe_consumer_handle,
    struct MojoDataPipeConsumerOptions* options,
    uint32_t options_num_bytes) {
  assert(g_thunks.GetDataPipeConsumerOptions);
  return g_thunks.GetDataPipeConsumerOptions(data_pipe_consumer_handle,
                                             options, options_num_bytes);
}

MojoResult MojoReadData(MojoHandle data_pipe_consumer_handle,
                        void* elements,
                        uint32_t* num_elements,
                        MojoReadDataFlags flags) {
  assert(g_thunks.ReadData);
  return g_thunks.ReadData(data_pipe_consumer_handle, elements, num_elements,
                           flags);
}

MojoResult MojoBeginReadData(MojoHandle data_pipe_consumer_handle,
                             const void** buffer,
                             uint32_t* buffer_num_elements,
                             MojoReadDataFlags flags) {
  assert(g_thunks.BeginReadData);
  return g_thunks.BeginReadData(data_pipe_consumer_handle, buffer,
                                buffer_num_elements, flags);
}

MojoResult MojoEndReadData(MojoHandle data_pipe_consumer_handle,
                           uint32_t num_elements_read) {
  assert(g_thunks.EndReadData);
  return g_thunks.EndReadData(data_pipe_consumer_handle, num_elements_read);
}

MojoResult MojoCreateSharedBuffer(
    const struct MojoCreateSharedBufferOptions* options,
    uint64_t num_bytes,
    MojoHandle* shared_buffer_handle) {
  assert(g_thunks.CreateSharedBuffer);
  return g_thunks.CreateSharedBuffer(options, num_bytes, shared_buffer_handle);
}

MojoResult MojoDuplicateBufferHandle(
    MojoHandle buffer_handle,
    const struct MojoDuplicateBufferHandleOptions* options,
    MojoHandle* new_buffer_handle) {
  assert(g_thunks.DuplicateBufferHandle);
  return g_thunks.DuplicateBufferHandle(buffer_handle, options,
                                        new_buffer_handle);
}

MojoResult MojoGetBufferInformation(MojoHandle buffer_handle,
                                    struct MojoBufferInformation* info,
                                    uint32_t info_num_bytes) {
  assert(g_thunks.GetBufferInformation);
  return g_thunks.GetBufferInformation(buffer_handle, info, info_num_bytes);
}

MojoResult MojoMapBuffer(MojoHandle buffer_handle,
                         uint64_t offset,
                         uint64_t num_bytes,
                         void** buffer,
                         MojoMapBufferFlags flags) {
  assert(g_thunks.MapBuffer);
  return g_thunks.MapBuffer(buffer_handle, offset, num_bytes, buffer, flags);
}

MojoResult MojoUnmapBuffer(void* buffer) {
  assert(g_thunks.UnmapBuffer);
  return g_thunks.UnmapBuffer(buffer);
}

THUNK_EXPORT size_t
MojoSetSystemThunks(const struct MojoSystemThunks* system_thunks) {
  if (system_thunks->size >= sizeof(g_thunks))
    g_thunks = *system_thunks;
  return sizeof(g_thunks);
}
