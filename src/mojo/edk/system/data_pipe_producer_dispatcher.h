// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_SYSTEM_DATA_PIPE_PRODUCER_DISPATCHER_H_
#define MOJO_EDK_SYSTEM_DATA_PIPE_PRODUCER_DISPATCHER_H_

#include "mojo/edk/system/dispatcher.h"
#include "mojo/edk/util/ref_ptr.h"
#include "mojo/edk/util/thread_annotations.h"
#include "mojo/public/cpp/system/macros.h"

namespace mojo {
namespace system {

class DataPipe;

// This is the |Dispatcher| implementation for the producer handle for data
// pipes (created by the Mojo primitive |MojoCreateDataPipe()|). This class is
// thread-safe.
class DataPipeProducerDispatcher final : public Dispatcher {
 public:
  // The default/standard rights for a data pipe consumer handle.
  static constexpr MojoHandleRights kDefaultHandleRights =
      MOJO_HANDLE_RIGHT_TRANSFER | MOJO_HANDLE_RIGHT_WRITE |
      MOJO_HANDLE_RIGHT_GET_OPTIONS | MOJO_HANDLE_RIGHT_SET_OPTIONS;

  static util::RefPtr<DataPipeProducerDispatcher> Create() {
    return AdoptRef(new DataPipeProducerDispatcher());
  }

  // Must be called before any other methods.
  void Init(util::RefPtr<DataPipe>&& data_pipe) MOJO_NOT_THREAD_SAFE;

  // |Dispatcher| public methods:
  Type GetType() const override;
  bool SupportsEntrypointClass(EntrypointClass entrypoint_class) const override;

  // The "opposite" of |SerializeAndClose()|. (Typically this is called by
  // |Dispatcher::Deserialize()|.)
  static util::RefPtr<DataPipeProducerDispatcher>
  Deserialize(Channel* channel, const void* source, size_t size);

  // Get access to the |DataPipe| for testing.
  DataPipe* GetDataPipeForTest();

 private:
  DataPipeProducerDispatcher();
  ~DataPipeProducerDispatcher() override;

  // |Dispatcher| protected methods:
  void CancelAllStateNoLock() override;
  void CloseImplNoLock() override;
  util::RefPtr<Dispatcher> CreateEquivalentDispatcherAndCloseImplNoLock(
      MessagePipe* message_pipe,
      unsigned port) override;
  MojoResult SetDataPipeProducerOptionsImplNoLock(
      UserPointer<const MojoDataPipeProducerOptions> options) override;
  MojoResult GetDataPipeProducerOptionsImplNoLock(
      UserPointer<MojoDataPipeProducerOptions> options,
      uint32_t options_num_bytes) override;
  MojoResult WriteDataImplNoLock(UserPointer<const void> elements,
                                 UserPointer<uint32_t> num_bytes,
                                 MojoWriteDataFlags flags) override;
  MojoResult BeginWriteDataImplNoLock(UserPointer<void*> buffer,
                                      UserPointer<uint32_t> buffer_num_bytes,
                                      MojoWriteDataFlags flags) override;
  MojoResult EndWriteDataImplNoLock(uint32_t num_bytes_written) override;
  HandleSignalsState GetHandleSignalsStateImplNoLock() const override;
  MojoResult AddAwakableImplNoLock(Awakable* awakable,
                                   MojoHandleSignals signals,
                                   bool force,
                                   uint64_t context,
                                   HandleSignalsState* signals_state) override;
  void RemoveAwakableImplNoLock(Awakable* awakable,
                                HandleSignalsState* signals_state) override;
  void RemoveAwakableWithContextImplNoLock(
      Awakable* awakable,
      uint64_t context,
      HandleSignalsState* signals_state) override;
  void StartSerializeImplNoLock(Channel* channel,
                                size_t* max_size,
                                size_t* max_platform_handles) override
      MOJO_NOT_THREAD_SAFE;
  bool EndSerializeAndCloseImplNoLock(
      Channel* channel,
      void* destination,
      size_t* actual_size,
      std::vector<platform::ScopedPlatformHandle>* platform_handles) override
      MOJO_NOT_THREAD_SAFE;

  // This will be null if closed.
  util::RefPtr<DataPipe> data_pipe_ MOJO_GUARDED_BY(mutex());

  MOJO_DISALLOW_COPY_AND_ASSIGN(DataPipeProducerDispatcher);
};

}  // namespace system
}  // namespace mojo

#endif  // MOJO_EDK_SYSTEM_DATA_PIPE_PRODUCER_DISPATCHER_H_
