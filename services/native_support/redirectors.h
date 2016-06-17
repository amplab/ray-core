// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NATIVE_SUPPORT_REDIRECTORS_H_
#define SERVICES_NATIVE_SUPPORT_REDIRECTORS_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_loop.h"
#include "mojo/public/cpp/bindings/array.h"
#include "mojo/services/files/interfaces/file.mojom.h"
#include "mojo/services/files/interfaces/types.mojom.h"

namespace native_support {

// Reads from an FD (as possible without blocking) and writes the result to a
// |mojo::files::File*|. This object must be used on an I/O thread.
class FDToMojoFileRedirector : public base::MessageLoopForIO::Watcher {
 public:
  // Both |fd| and |file| must outlive this object.
  FDToMojoFileRedirector(int fd, mojo::files::File* file, size_t buffer_size);
  ~FDToMojoFileRedirector() override;

  void Start();
  void Stop();

 private:
  // |base::MessageLoopForIO::Watcher| implementation:
  void OnFileCanReadWithoutBlocking(int fd) override;
  void OnFileCanWriteWithoutBlocking(int fd) override;

  void DoWrite();
  void DidWrite(mojo::files::Error error, uint32_t num_bytes_written);

  const int fd_;
  mojo::files::File* const file_;
  const size_t buffer_size_;
  std::unique_ptr<char[]> buffer_;
  base::MessageLoopForIO::FileDescriptorWatcher watcher_;

  size_t num_bytes_;
  size_t offset_;

  base::WeakPtrFactory<FDToMojoFileRedirector> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(FDToMojoFileRedirector);
};

// Reads from a |mojo::files::File*| and writes the result to an FD.
// TODO(vtl): This is very cheesy for now. It just sets |fd| to non-blocking,
// and assumes that we'll never block on writing to it. (This is mostly "OK" for
// interactive input redirection.)
class MojoFileToFDRedirector {
 public:
  // Both |file| and |fd| must outlive this object.
  MojoFileToFDRedirector(mojo::files::File* file, int fd, size_t buffer_size);
  ~MojoFileToFDRedirector();

  void Start();
  void Stop();

 private:
  void DidRead(mojo::files::Error error, mojo::Array<uint8_t> bytes_read);

  mojo::files::File* const file_;
  const int fd_;
  const size_t buffer_size_;
  bool running_;
  bool read_pending_;

  base::WeakPtrFactory<MojoFileToFDRedirector> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MojoFileToFDRedirector);
};

}  // namespace native_support

#endif  // SERVICES_NATIVE_SUPPORT_REDIRECTORS_H_
