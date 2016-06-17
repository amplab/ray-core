// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/native_support/redirectors.h"

#include <errno.h>
#include <string.h>

#include "base/bind.h"
#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"

namespace native_support {

// FDToMojoFileRedirector ------------------------------------------------------

FDToMojoFileRedirector::FDToMojoFileRedirector(int fd,
                                               mojo::files::File* file,
                                               size_t buffer_size)
    : fd_(fd),
      file_(file),
      buffer_size_(buffer_size),
      num_bytes_(0),
      offset_(0),
      weak_factory_(this) {
  DCHECK_NE(fd_, -1);
  DCHECK(file_);
}

FDToMojoFileRedirector::~FDToMojoFileRedirector() {}

void FDToMojoFileRedirector::Start() {
  // One-shot watch (since we'll need to wait for write callbacks).
  bool success = base::MessageLoopForIO::current()->WatchFileDescriptor(
      fd_, false, base::MessageLoopForIO::WATCH_READ, &watcher_, this);
  DCHECK(success);
}

void FDToMojoFileRedirector::Stop() {
  watcher_.StopWatchingFileDescriptor();
}

void FDToMojoFileRedirector::OnFileCanReadWithoutBlocking(int fd) {
  DCHECK_EQ(fd, fd_);

  if (!buffer_)
    buffer_.reset(new char[buffer_size_]);

  ssize_t result = HANDLE_EINTR(read(fd_, buffer_.get(), buffer_size_));
  if (result < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      LOG(WARNING) << "Could not read from FD without blocking";
      Start();
      return;
    }
    PLOG(WARNING) << "Failed to read from FD";
    // TODO(vtl): Should maybe close |file_|?
    return;
  }
  if (!result) {  // EOF.
    // TODO(vtl): Should maybe close |file_|?
    return;
  }

  num_bytes_ = static_cast<size_t>(result);
  offset_ = 0;
  DoWrite();
}

void FDToMojoFileRedirector::OnFileCanWriteWithoutBlocking(int /*fd*/) {
  NOTREACHED();
}

void FDToMojoFileRedirector::DoWrite() {
  CHECK_GT(num_bytes_, offset_);
  size_t num_bytes_to_write = num_bytes_ - offset_;

  // TODO(vtl): Is there a more natural (or efficient) way to do this?
  auto bytes_to_write = mojo::Array<uint8_t>::New(num_bytes_to_write);
  memcpy(&bytes_to_write[offset_], buffer_.get(), num_bytes_to_write);

  file_->Write(bytes_to_write.Pass(), 0, mojo::files::Whence::FROM_CURRENT,
               base::Bind(&FDToMojoFileRedirector::DidWrite,
                          weak_factory_.GetWeakPtr()));
}

void FDToMojoFileRedirector::DidWrite(mojo::files::Error error,
                                      uint32_t num_bytes_written) {
  if (error != mojo::files::Error::OK) {
    LOG(WARNING) << "Failed to write to Mojo File";
    // TODO(vtl): Should maybe close |file_|?
    return;
  }

  CHECK_GT(num_bytes_, offset_);
  size_t num_bytes_to_write = num_bytes_ - offset_;
  if (num_bytes_written > num_bytes_to_write) {
    LOG(ERROR) << "Bad result from write to Mojo File";
    return;
  }

  offset_ += num_bytes_written;
  if (offset_ < num_bytes_) {
    DoWrite();
  } else {
    num_bytes_ = 0;
    offset_ = 0;
    Start();
  }
}

// MojoFileToFDRedirector ------------------------------------------------------

MojoFileToFDRedirector::MojoFileToFDRedirector(mojo::files::File* file,
                                               int fd,
                                               size_t buffer_size)
    : file_(file),
      fd_(fd),
      buffer_size_(buffer_size),
      running_(false),
      read_pending_(false),
      weak_factory_(this) {}

MojoFileToFDRedirector::~MojoFileToFDRedirector() {}

void MojoFileToFDRedirector::Start() {
  running_ = true;

  if (read_pending_)
    return;

  file_->Read(
      static_cast<uint32_t>(buffer_size_), 0, mojo::files::Whence::FROM_CURRENT,
      base::Bind(&MojoFileToFDRedirector::DidRead, weak_factory_.GetWeakPtr()));
  read_pending_ = true;
}

void MojoFileToFDRedirector::Stop() {
  running_ = false;
}

void MojoFileToFDRedirector::DidRead(mojo::files::Error error,
                                     mojo::Array<uint8_t> bytes_read) {
  DCHECK(read_pending_);
  read_pending_ = false;

  if (error != mojo::files::Error::OK) {
    LOG(ERROR) << "Read failed";
    // TODO(vtl): Should maybe close |file_|?
    return;
  }

  ssize_t result = HANDLE_EINTR(write(fd_, &bytes_read[0], bytes_read.size()));
  if (result < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      LOG(ERROR) << "Could not write to FD without blocking";
      if (running_)
        Start();
      return;
    }
    PLOG(WARNING) << "Failed to write to FD";
    // TODO(vtl): Should maybe close |file_|?
    return;
  }
  if (static_cast<size_t>(result) != bytes_read.size())
    LOG(ERROR) << "Failed to write everything to FD";

  if (running_)
    Start();
}

}  // namespace native_support
