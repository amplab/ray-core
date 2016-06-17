// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_SERVICES_FLOG_FLOG_READER_IMPL_H_
#define MOJO_SERVICES_FLOG_FLOG_READER_IMPL_H_

#include <limits>

#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/services/flog/interfaces/flog.mojom.h"
#include "services/flog/flog_service_impl.h"

namespace mojo {
namespace flog {

// FlogReader implementation.
class FlogReaderImpl : public FlogServiceImpl::Product<FlogReader>,
                       public FlogReader,
                       private FlogLogger {
 public:
  static std::shared_ptr<FlogReaderImpl> Create(
      InterfaceRequest<FlogReader> request,
      uint32_t log_id,
      const std::string& label,
      std::shared_ptr<FlogDirectory> directory,
      FlogServiceImpl* owner);

  ~FlogReaderImpl() override;

  // FlogReader implementation.
  void GetEntries(uint32_t start_index,
                  uint32_t max_count,
                  const GetEntriesCallback& callback) override;

 private:
  static const size_t kReadBufferSize = 16 * 1024;

  FlogReaderImpl(InterfaceRequest<FlogReader> request,
                 uint32_t log_id,
                 const std::string& label,
                 std::shared_ptr<FlogDirectory> directory,
                 FlogServiceImpl* owner);

  bool DiscardEntry();

  FlogEntryPtr GetEntry();

  // Reads data and returns the number of bytes read. |data_size| must be
  // non-zero. |data| may be nullptr, in which case the data is discarded.
  size_t ReadData(size_t data_size, void* data);

  void FillReadBuffer(bool restart);

  size_t read_buffer_bytes_remaining() {
    return read_buffer_.size() - read_buffer_bytes_used_;
  }

  // Creates an entry with an uninitialized details field.
  FlogEntryPtr CreateEntry(int64_t time_us, uint32_t channel_id);

  // FlogLogger implementation (called by stub_).
  void LogChannelCreation(int64_t time_us,
                          uint32_t channel_id,
                          const mojo::String& type_name) override;

  void LogChannelMessage(int64_t time_us,
                         uint32_t channel_id,
                         mojo::Array<uint8_t> data) override;

  void LogChannelDeletion(int64_t time_us, uint32_t channel_id) override;

  uint32_t log_id_;
  files::FilePtr file_;
  uint32_t current_entry_index_ = 0;
  std::vector<uint8_t> read_buffer_;
  size_t read_buffer_bytes_used_;
  bool fault_ = false;
  FlogLoggerStub stub_;
  FlogEntryPtr entry_;
};

}  // namespace flog
}  // namespace mojo

#endif  // MOJO_SERVICES_FLOG_FLOG_READER_IMPL_H_
