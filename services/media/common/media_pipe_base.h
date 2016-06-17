// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_MEDIA_COMMON_MEDIA_PIPE_BASE_H_
#define SERVICES_MEDIA_COMMON_MEDIA_PIPE_BASE_H_

#include <atomic>
#include <deque>
#include <memory>

#include "base/synchronization/lock.h"
#include "base/task_runner.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/system/buffer.h"
#include "mojo/services/media/common/interfaces/media_transport.mojom.h"

namespace mojo {
namespace media {

class MediaPipeBase : public MediaConsumer {
 protected:
  class MappedSharedBuffer;
  using MappedSharedBufferPtr = std::shared_ptr<MappedSharedBuffer>;

 public:
  class MediaPacketState {
   public:
    ~MediaPacketState();

    const MediaPacketPtr& packet() const { return packet_; }
    const MappedSharedBufferPtr& buffer() const { return buffer_; }
    void SetResult(MediaConsumer::SendResult result);

   private:
    friend class MediaPipeBase;
    MediaPacketState(MediaPacketPtr packet,
                     const MappedSharedBufferPtr& buffer,
                     const SendPacketCallback& cbk);

    MediaPacketPtr packet_;
    MappedSharedBufferPtr buffer_;
    SendPacketCallback cbk_;
    std::atomic<MediaConsumer::SendResult> result_;
  };
  using MediaPacketStatePtr = std::unique_ptr<MediaPacketState>;

  // Default constructor and destructor
  MediaPipeBase();
  ~MediaPipeBase() override;

  // Initialize the internal state of the pipe (allocate resources, etc..)
  MojoResult Init(InterfaceRequest<MediaConsumer> request);

  bool IsInitialized() const;
  void Reset();

 protected:
  class MappedSharedBuffer {
   public:
    static MappedSharedBufferPtr Create(ScopedSharedBufferHandle handle,
                                        uint64_t size);
    ~MappedSharedBuffer();

    const ScopedSharedBufferHandle& handle() const { return handle_; }
    uint64_t size() const { return size_; }
    void*    base() const { return base_; }

   private:
    MappedSharedBuffer(ScopedSharedBufferHandle handle, uint64_t size);

    ScopedSharedBufferHandle handle_;
    uint64_t size_;
    void*  base_ = nullptr;
  };

  // Interface to be implemented by derived classes
  virtual void OnPacketReceived(MediaPacketStatePtr state) = 0;
  virtual void OnPrimeRequested(const PrimeCallback& cbk) = 0;
  virtual bool OnFlushRequested(const FlushCallback& cbk) = 0;

  MappedSharedBufferPtr buffer_;

 private:
  Binding<MediaConsumer> binding_;

  // MediaConsumer.mojom implementation.
  void SetBuffer(ScopedSharedBufferHandle handle,
                const SetBufferCallback& cbk) final;
  void SendPacket(MediaPacketPtr packet,
                  const SendPacketCallback& cbk) final;
  void Prime(const PrimeCallback& cbk) final;
  void Flush(const FlushCallback& cbk) final;
};

}  // namespace media
}  // namespace mojo

#endif  // SERVICES_MEDIA_COMMON_MEDIA_PIPE_BASE_H_
