// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// |MotermDriver| is a class providing a |mojo.files.File| interface,
// implementing termios-type features (e.g., line editing; TODO(vtl): lots to do
// here), and appropriately processing bytes to/from the terminal (which gets
// and sends "raw" data). In essence, this class includes what would
// traditionally be called the terminal driver in a Unix kernel.

#ifndef APPS_MOTERM_MOTERM_DRIVER_H_
#define APPS_MOTERM_MOTERM_DRIVER_H_

#include <stddef.h>
#include <stdint.h>

#include <deque>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/files/interfaces/file.mojom.h"
#include "mojo/services/files/interfaces/types.mojom.h"

// TODO(vtl): Maybe we should Mojo-fy the driver and run it as a separate app?
class MotermDriver : public mojo::files::File {
 public:
  // The |Client| is basically the terminal implementation itself, to which
  // processed output (from an application, to the "file", through the driver)
  // is sent. It also receives other notifications (e.g., when the "file" is
  // closed or otherwise "broken").
  class Client {
   public:
    // Called when we receive data (for the client to "display").
    // TODO(vtl): Maybe add a way to throttle (e.g., for the client to not
    // accept all the data).
    // TODO(vtl): It's "probably OK" to call |Detach()| from inside this method,
    // but not verified.
    virtual void OnDataReceived(const void* bytes, size_t num_bytes) = 0;

    // Called when the terminal "file" is closed (via |Close()|). (The client
    // may optionally call |Detach()| in response.)
    virtual void OnClosed() = 0;

    // Called when this object is destroyed (which will happen if the other end
    // of the message pipe is closed). The client must not call |Detach()| in
    // response. (Obviously, this will only be called if the client has not
    // called |Detach()|.)
    virtual void OnDestroyed() = 0;
  };

  // Static factory method. |client| must either outlive us, or if not call
  // |Detach()| before being destroyed.
  static base::WeakPtr<MotermDriver> Create(
      Client* client,
      mojo::InterfaceRequest<mojo::files::File> request);

  // Called by the client when it wishes to detach (i.e., no longer receive
  // calls). This will terminate the connection and us.
  void Detach();

  // Called by the client when it has data to send.
  // TODO(vtl): Add a way to throttle. (We could do this via an |OnDataSent()|
  // ack, or in other ways as well, e.g., reporting how much data we have
  // enqueued.) For now, just queue data infinitely.
  // TODO(vtl): This is misnamed -- it's really "here's some input".
  void SendData(const void* bytes, size_t num_bytes);

 private:
  struct PendingRead {
    PendingRead(uint32_t num_bytes, const ReadCallback& callback);
    ~PendingRead();

    uint32_t num_bytes;
    ReadCallback callback;
  };

  MotermDriver(Client* client,
               mojo::InterfaceRequest<mojo::files::File> request);

  // We should only be deleted by "ourself" (via the strong binding).
  friend class mojo::StrongBinding<mojo::files::File>;
  ~MotermDriver() override;

  void HandleCanonicalModeInput(uint8_t ch);
  void CompletePendingReads();
  void FlushInputLine();

  void HandleOutput(const uint8_t* bytes, size_t num_bytes);

  // |mojo::files::File| implementation:
  void Close(const CloseCallback& callback) override;
  void Read(uint32_t num_bytes_to_read,
            int64_t offset,
            mojo::files::Whence whence,
            const ReadCallback& callback) override;
  void Write(mojo::Array<uint8_t> bytes_to_write,
             int64_t offset,
             mojo::files::Whence whence,
             const WriteCallback& callback) override;
  void ReadToStream(mojo::ScopedDataPipeProducerHandle source,
                    int64_t offset,
                    mojo::files::Whence whence,
                    int64_t num_bytes_to_read,
                    const ReadToStreamCallback& callback) override;
  void WriteFromStream(mojo::ScopedDataPipeConsumerHandle sink,
                       int64_t offset,
                       mojo::files::Whence whence,
                       const WriteFromStreamCallback& callback) override;
  void Tell(const TellCallback& callback) override;
  void Seek(int64_t offset,
            mojo::files::Whence whence,
            const SeekCallback& callback) override;
  void Stat(const StatCallback& callback) override;
  void Truncate(int64_t size, const TruncateCallback& callback) override;
  void Touch(mojo::files::TimespecOrNowPtr atime,
             mojo::files::TimespecOrNowPtr mtime,
             const TouchCallback& callback) override;
  void Dup(mojo::InterfaceRequest<mojo::files::File> file,
           const DupCallback& callback) override;
  void Reopen(mojo::InterfaceRequest<mojo::files::File> file,
              uint32_t open_flags,
              const ReopenCallback& callback) override;
  void AsBuffer(const AsBufferCallback& callback) override;
  void Ioctl(uint32_t request,
             mojo::Array<uint32_t> in_values,
             const IoctlCallback& callback) override;

  // Helpers for |Ioctl()|:
  void IoctlGetSettings(mojo::Array<uint32_t> in_values,
                        const IoctlCallback& callback);
  void IoctlSetSettings(mojo::Array<uint32_t> in_values,
                        const IoctlCallback& callback);
  mojo::files::Error IoctlSetSettingsHelper(mojo::Array<uint32_t> in_values);

  Client* client_;  // Set until |Detach()| is called.
  bool is_closed_;

  std::deque<uint8_t> send_data_queue_;
  // For canonical mode. Feeds into |send_data_queue_|.
  std::deque<uint8_t> input_line_queue_;
  std::deque<PendingRead> pending_read_queue_;

  // Note: This binding must be after |pending_read_queue_|, so that it gets
  // torn down before the callbacks in |pending_read_queue_| are destroyed
  // (otherwise we'll get |DCHECK()| failures).
  mojo::StrongBinding<mojo::files::File> binding_;

  // Terminal driver settings:
  // (Names roughly correspond to termios names.)
  // TODO(vtl): Add a way to set/change settings (including using ioctls).
  // TODO(vtl): Our support for termios requirements is very far from complete.

  // Input settings:

  // Canonical, a.k.a. "cooked", mode. If true, will echo, line buffer, etc. If
  // false, no input processing is done (other than perhaps to slightly
  // time-delay the availability of input -- we don't do this).
  bool icanon_;

  // If true, will convert input CRs to NLs (in canonical mode).
  bool icrnl_;

  uint8_t veof_;
  uint8_t verase_;

  // Output settings:
  // If true, will convert output CRs to CR-NL pairs.
  bool onlcr_;

  base::WeakPtrFactory<MotermDriver> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MotermDriver);
};

#endif  // APPS_MOTERM_MOTERM_DRIVER_H_
