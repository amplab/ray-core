// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/moterm/moterm_driver.h"

#include <algorithm>
#include <limits>

#include "base/logging.h"
#include "mojo/services/files/interfaces/ioctl.mojom.h"
#include "mojo/services/files/interfaces/ioctl_terminal.mojom.h"

// Character constants:
const uint8_t kEOT = 4;
const uint8_t kNL = 10;
const uint8_t kCR = 13;
const uint8_t kDEL = 127;

// Short forms for various counts/indices used for the get/set settings ioctls.
const size_t kBaseFieldCount = mojo::files::kIoctlTerminalTermiosBaseFieldCount;
const size_t kTotalFieldCount =
    kBaseFieldCount + mojo::files::kIoctlTerminalTermiosCtrlCharCount;
const size_t kIFlagIdx = mojo::files::kIoctlTerminalTermiosIFlagIndex;
const size_t kOFlagIdx = mojo::files::kIoctlTerminalTermiosOFlagIndex;
const size_t kLFlagIdx = mojo::files::kIoctlTerminalTermiosLFlagIndex;
const size_t kVEraseIdx = mojo::files::kIoctlTerminalTermiosCtrlCharVERASEIndex;
const size_t kVEOFIdx = mojo::files::kIoctlTerminalTermiosCtrlCharVERASEIndex;

MotermDriver::PendingRead::PendingRead(uint32_t num_bytes,
                                       const ReadCallback& callback)
    : num_bytes(num_bytes), callback(callback) {
}

MotermDriver::PendingRead::~PendingRead() {
}

// static
base::WeakPtr<MotermDriver> MotermDriver::Create(
    Client* client,
    mojo::InterfaceRequest<mojo::files::File> request) {
  return (new MotermDriver(client, request.Pass()))->weak_factory_.GetWeakPtr();
}

void MotermDriver::Detach() {
  client_ = nullptr;
  delete this;
}

void MotermDriver::SendData(const void* bytes, size_t num_bytes) {
  DCHECK(client_);
  DCHECK(!is_closed_);

  if (icanon_) {
    for (size_t i = 0; i < num_bytes; i++)
      HandleCanonicalModeInput(static_cast<const uint8_t*>(bytes)[i]);
  } else {
    // If not in canonical ("cooked") mode, just send data.
    const uint8_t* b = static_cast<const uint8_t*>(bytes);
    for (size_t i = 0; i < num_bytes; i++)
      send_data_queue_.push_back(b[i]);

    CompletePendingReads();
  }
}

MotermDriver::MotermDriver(Client* client,
                           mojo::InterfaceRequest<mojo::files::File> request)
    : client_(client),
      is_closed_(false),
      binding_(this, request.Pass()),
      icanon_(true),
      icrnl_(true),
      veof_(kEOT),
      verase_(kDEL),
      onlcr_(true),
      weak_factory_(this) {
  DCHECK(client_);
}

MotermDriver::~MotermDriver() {
  if (client_) {
    // Invalidate weak pointers now, to make sure the client doesn't call us.
    weak_factory_.InvalidateWeakPtrs();
    client_->OnDestroyed();
  }
}

void MotermDriver::HandleCanonicalModeInput(uint8_t ch) {
  DCHECK(icanon_);

  // Translate CR to NL, if appropriate.
  if (ch == kCR && icrnl_)
    ch = kNL;

  // Newline.
  // TODO(vtl): In addition to NL, we're supposed to handle settable EOL and
  // EOL2.
  if (ch == kNL) {  // Newline.
    input_line_queue_.push_back(ch);
    HandleOutput(&ch, 1);
    FlushInputLine();
    return;
  }

  // EOF at beginning of line.
  if (ch == veof_ && input_line_queue_.empty()) {
    // Don't add the character. Just flush and nuke ourselves.
    FlushInputLine();
    delete this;
    return;
  }

  if (ch == verase_) {  // Erase (backspace).
    if (!input_line_queue_.empty()) {
      // TODO(vtl): This is wrong for utf-8 (see Linux's IUTF8).
      input_line_queue_.pop_back();
      HandleOutput(reinterpret_cast<const uint8_t*>("\x08 \x08"), 3);
    }  // Else do nothing.
    return;
  }

  // Everything else.
  // TODO(vtl): Probably want to display control characters as ^X (e.g.).
  // TODO(vtl): Handle ^W.
  input_line_queue_.push_back(ch);
  HandleOutput(&ch, 1);
}

void MotermDriver::CompletePendingReads() {
  while (send_data_queue_.size() && pending_read_queue_.size()) {
    PendingRead pending_read = pending_read_queue_.front();
    pending_read_queue_.pop_front();

    size_t data_size = std::min(static_cast<size_t>(pending_read.num_bytes),
                                send_data_queue_.size());
    auto data = mojo::Array<uint8_t>::New(data_size);
    for (size_t i = 0; i < data_size; i++) {
      data[i] = send_data_queue_[i];
      // In canonical mode, each read only gets a single line.
      if (icanon_ && data[i] == kNL) {
        data_size = i + 1;
        data.resize(data_size);
        break;
      }
    }
    send_data_queue_.erase(send_data_queue_.begin(),
                           send_data_queue_.begin() + data_size);

    pending_read.callback.Run(mojo::files::Error::OK, data.Pass());
  }
}

void MotermDriver::FlushInputLine() {
  for (size_t i = 0; i < input_line_queue_.size(); i++)
    send_data_queue_.push_back(input_line_queue_[i]);
  input_line_queue_.clear();
  CompletePendingReads();
}

void MotermDriver::HandleOutput(const uint8_t* bytes, size_t num_bytes) {
  // Can we be smarter and not always copy?
  std::vector<uint8_t> translated_bytes;
  translated_bytes.reserve(num_bytes);

  for (size_t i = 0; i < num_bytes; i++) {
    uint8_t ch = bytes[i];
    if (ch == kNL && onlcr_) {
      translated_bytes.push_back(kCR);
      translated_bytes.push_back(kNL);
    } else {
      translated_bytes.push_back(ch);
    }
  }

  // TODO(vtl): It seems extremely unlikely that we'd overlow a |uint32_t| here
  // (but it's theoretically possible). But perhaps we should handle it more
  // gracefully if it ever comes up.
  CHECK_LE(translated_bytes.size(), std::numeric_limits<uint32_t>::max());
  client_->OnDataReceived(translated_bytes.data(),
                          static_cast<uint32_t>(translated_bytes.size()));
}

void MotermDriver::Close(const CloseCallback& callback) {
  if (is_closed_) {
    callback.Run(mojo::files::Error::CLOSED);
    return;
  }

  // TODO(vtl): Call pending read callbacks?

  is_closed_ = true;
  callback.Run(mojo::files::Error::OK);

  client_->OnClosed();
}

void MotermDriver::Read(uint32_t num_bytes_to_read,
                        int64_t offset,
                        mojo::files::Whence whence,
                        const ReadCallback& callback) {
  if (is_closed_) {
    callback.Run(mojo::files::Error::CLOSED, nullptr);
    return;
  }

  if (offset != 0 || whence != mojo::files::Whence::FROM_CURRENT) {
    // TODO(vtl): Is this the "right" behavior?
    callback.Run(mojo::files::Error::INVALID_ARGUMENT, nullptr);
    return;
  }

  if (!num_bytes_to_read) {
    callback.Run(mojo::files::Error::OK, nullptr);
    return;
  }

  pending_read_queue_.push_back(PendingRead(num_bytes_to_read, callback));
  CompletePendingReads();
}

void MotermDriver::Write(mojo::Array<uint8_t> bytes_to_write,
                         int64_t offset,
                         mojo::files::Whence whence,
                         const WriteCallback& callback) {
  DCHECK(!bytes_to_write.is_null());

  if (is_closed_) {
    callback.Run(mojo::files::Error::CLOSED, 0);
    return;
  }

  if (offset != 0 || whence != mojo::files::Whence::FROM_CURRENT) {
    // TODO(vtl): Is this the "right" behavior?
    callback.Run(mojo::files::Error::INVALID_ARGUMENT, 0);
    return;
  }

  if (!bytes_to_write.size()) {
    callback.Run(mojo::files::Error::OK, 0);
    return;
  }

  HandleOutput(&bytes_to_write.front(), bytes_to_write.size());

  // TODO(vtl): Is this OK if the client detached (and we're destroyed?).
  callback.Run(mojo::files::Error::OK,
               static_cast<uint32_t>(bytes_to_write.size()));
}

void MotermDriver::ReadToStream(mojo::ScopedDataPipeProducerHandle source,
                                int64_t offset,
                                mojo::files::Whence whence,
                                int64_t num_bytes_to_read,
                                const ReadToStreamCallback& callback) {
  if (is_closed_) {
    callback.Run(mojo::files::Error::CLOSED);
    return;
  }

  // TODO(vtl)
  NOTIMPLEMENTED();
  callback.Run(mojo::files::Error::UNIMPLEMENTED);
}

void MotermDriver::WriteFromStream(mojo::ScopedDataPipeConsumerHandle sink,
                                   int64_t offset,
                                   mojo::files::Whence whence,
                                   const WriteFromStreamCallback& callback) {
  if (is_closed_) {
    callback.Run(mojo::files::Error::CLOSED);
    return;
  }

  // TODO(vtl)
  NOTIMPLEMENTED();
  callback.Run(mojo::files::Error::UNIMPLEMENTED);
}

void MotermDriver::Tell(const TellCallback& callback) {
  if (is_closed_) {
    callback.Run(mojo::files::Error::CLOSED, 0);
    return;
  }

  // TODO(vtl): Is this what we want? (Also is "unavailable" right? Maybe
  // unsupported/EINVAL is better.)
  callback.Run(mojo::files::Error::UNAVAILABLE, 0);
}

void MotermDriver::Seek(int64_t offset,
                        mojo::files::Whence whence,
                        const SeekCallback& callback) {
  if (is_closed_) {
    callback.Run(mojo::files::Error::CLOSED, 0);
    return;
  }

  // TODO(vtl): Is this what we want? (Also is "unavailable" right? Maybe
  // unsupported/EINVAL is better.)
  callback.Run(mojo::files::Error::UNAVAILABLE, 0);
}

void MotermDriver::Stat(const StatCallback& callback) {
  if (is_closed_) {
    callback.Run(mojo::files::Error::CLOSED, nullptr);
    return;
  }

  // TODO(vtl)
  NOTIMPLEMENTED();
  callback.Run(mojo::files::Error::UNIMPLEMENTED, nullptr);
}

void MotermDriver::Truncate(int64_t size, const TruncateCallback& callback) {
  if (is_closed_) {
    callback.Run(mojo::files::Error::CLOSED);
    return;
  }

  // TODO(vtl): Is this what we want? (Also is "unavailable" right? Maybe
  // unsupported/EINVAL is better.)
  callback.Run(mojo::files::Error::UNAVAILABLE);
}

void MotermDriver::Touch(mojo::files::TimespecOrNowPtr atime,
                         mojo::files::TimespecOrNowPtr mtime,
                         const TouchCallback& callback) {
  if (is_closed_) {
    callback.Run(mojo::files::Error::CLOSED);
    return;
  }

  // TODO(vtl): Is this what we want? (Also is "unavailable" right? Maybe
  // unsupported/EINVAL is better.)
  callback.Run(mojo::files::Error::UNAVAILABLE);
}

void MotermDriver::Dup(mojo::InterfaceRequest<mojo::files::File> file,
                       const DupCallback& callback) {
  if (is_closed_) {
    callback.Run(mojo::files::Error::CLOSED);
    return;
  }

  // TODO(vtl): Is this what we want? (Also is "unavailable" right? Maybe
  // unsupported/EINVAL is better.)
  callback.Run(mojo::files::Error::UNAVAILABLE);
}

void MotermDriver::Reopen(mojo::InterfaceRequest<mojo::files::File> file,
                          uint32_t open_flags,
                          const ReopenCallback& callback) {
  if (is_closed_) {
    callback.Run(mojo::files::Error::CLOSED);
    return;
  }

  // TODO(vtl): Is this what we want? (Also is "unavailable" right? Maybe
  // unsupported/EINVAL is better.)
  callback.Run(mojo::files::Error::UNAVAILABLE);
}

void MotermDriver::AsBuffer(const AsBufferCallback& callback) {
  if (is_closed_) {
    callback.Run(mojo::files::Error::CLOSED, mojo::ScopedSharedBufferHandle());
    return;
  }

  // TODO(vtl): Is this what we want? (Also is "unavailable" right? Maybe
  // unsupported/EINVAL is better.)
  callback.Run(mojo::files::Error::UNAVAILABLE,
               mojo::ScopedSharedBufferHandle());
}

void MotermDriver::Ioctl(uint32_t request,
                         mojo::Array<uint32_t> in_values,
                         const IoctlCallback& callback) {
  if (is_closed_) {
    callback.Run(mojo::files::Error::CLOSED, nullptr);
    return;
  }

  if (request != mojo::files::kIoctlTerminal) {
    callback.Run(mojo::files::Error::UNIMPLEMENTED, nullptr);
    return;
  }

  // "Is TTY?" Yes.
  if (!in_values || !in_values.size()) {
    callback.Run(mojo::files::Error::OK, nullptr);
    return;
  }

  switch (in_values[0]) {
    case mojo::files::kIoctlTerminalGetSettings:
      IoctlGetSettings(in_values.Pass(), callback);
      return;
    case mojo::files::kIoctlTerminalSetSettings:
      IoctlSetSettings(in_values.Pass(), callback);
      return;
    case mojo::files::kIoctlTerminalGetWindowSize:
    case mojo::files::kIoctlTerminalSetWindowSize:
      NOTIMPLEMENTED();
      callback.Run(mojo::files::Error::UNIMPLEMENTED, nullptr);
      return;
    default:
      callback.Run(mojo::files::Error::UNIMPLEMENTED, nullptr);
      return;
  }
}

void MotermDriver::IoctlGetSettings(mojo::Array<uint32_t> in_values,
                                    const IoctlCallback& callback) {
  if (in_values.size() != 1u) {
    callback.Run(mojo::files::Error::INVALID_ARGUMENT, nullptr);
    return;
  }

  auto out_values = mojo::Array<uint32_t>::New(kTotalFieldCount);

  // TODO(vtl): Add support for various things. Also, some values should be
  // hard-coded.

  // iflag:
  if (icrnl_)
    out_values[kIFlagIdx] |= mojo::files::kIoctlTerminalTermiosIFlagICRNL;

  // oflag:
  if (onlcr_)
    out_values[kOFlagIdx] |= mojo::files::kIoctlTerminalTermiosOFlagONLCR;

  // lflag:
  if (icanon_)
    out_values[kLFlagIdx] |= mojo::files::kIoctlTerminalTermiosLFlagICANON;

  // cc:
  out_values[kVEraseIdx] = verase_;
  out_values[kVEOFIdx] = veof_;

  callback.Run(mojo::files::Error::OK, out_values.Pass());
}

void MotermDriver::IoctlSetSettings(mojo::Array<uint32_t> in_values,
                                    const IoctlCallback& callback) {
  callback.Run(IoctlSetSettingsHelper(in_values.Pass()), nullptr);
}

mojo::files::Error MotermDriver::IoctlSetSettingsHelper(
    mojo::Array<uint32_t> in_values) {
  // Note: "termios" offsets (and sizes) are increased by 1, to accomodate the
  // subrequest at index 0.

  // The "cc" values are optional.
  if (in_values.size() < 1 + kBaseFieldCount)
    return mojo::files::Error::INVALID_ARGUMENT;

  // TODO(vtl): Add support for various things. Also, some values can't be
  // changed.

  // iflag:
  icrnl_ = !!(in_values[1 + kIFlagIdx] &
              mojo::files::kIoctlTerminalTermiosIFlagICRNL);

  // oflag:
  onlcr_ = !!(in_values[1 + kOFlagIdx] &
              mojo::files::kIoctlTerminalTermiosOFlagONLCR);

  // lflag:
  icanon_ = !!(in_values[1 + kLFlagIdx] &
               mojo::files::kIoctlTerminalTermiosLFlagICANON);

  // TODO(vtl): Check that ispeed and ospeed are not set?

  // cc:
  if (1 + kVEraseIdx < in_values.size()) {
    uint32_t value = in_values[1 + kVEraseIdx];
    if (value > std::numeric_limits<uint8_t>::max())
      return mojo::files::Error::INVALID_ARGUMENT;
    verase_ = static_cast<uint8_t>(value);
  }
  if (1 + kVEOFIdx < in_values.size()) {
    uint32_t value = in_values[1 + kVEOFIdx];
    if (value > std::numeric_limits<uint8_t>::max())
      return mojo::files::Error::INVALID_ARGUMENT;
    veof_ = static_cast<uint8_t>(value);
  }

  return mojo::files::Error::OK;
}
