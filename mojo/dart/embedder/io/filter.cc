// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/dart/embedder/io/filter.h"

namespace mojo {
namespace dart {

const int kZLibFlagUseGZipHeader = 16;
const int kZLibFlagAcceptAnyHeader = 32;

static const int kFilterPointerNativeField = 0;

static void DeleteFilter(
    void* isolate_data,
    Dart_WeakPersistentHandle handle,
    void* filter_pointer) {
  Filter* filter = reinterpret_cast<Filter*>(filter_pointer);
  delete filter;
}

Dart_Handle Filter::SetFilterAndCreateFinalizer(Dart_Handle filter,
                                                Filter* filter_pointer,
                                                intptr_t size) {
  Dart_Handle err = Dart_SetNativeInstanceField(
      filter,
      kFilterPointerNativeField,
      reinterpret_cast<intptr_t>(filter_pointer));
  if (Dart_IsError(err)) {
    return err;
  }
  Dart_NewWeakPersistentHandle(filter,
                               reinterpret_cast<void*>(filter_pointer),
                               size,
                               DeleteFilter);
  return err;
}

Dart_Handle Filter::GetFilterNativeField(Dart_Handle filter,
                                         Filter** filter_pointer) {
  return Dart_GetNativeInstanceField(
      filter,
      kFilterPointerNativeField,
      reinterpret_cast<intptr_t*>(filter_pointer));
}

ZLibDeflateFilter::~ZLibDeflateFilter() {
  delete[] dictionary_;
  delete[] current_buffer_;
  if (initialized()) {
    deflateEnd(&stream_);
  }
}

bool ZLibDeflateFilter::Init() {
  int window_bits = window_bits_;
  if (raw_) {
    window_bits = -window_bits;
  } else if (gzip_) {
    window_bits += kZLibFlagUseGZipHeader;
  }
  stream_.next_in = Z_NULL;
  stream_.zalloc = Z_NULL;
  stream_.zfree = Z_NULL;
  stream_.opaque = Z_NULL;
  int result = deflateInit2(&stream_, level_, Z_DEFLATED, window_bits,
                            mem_level_, strategy_);
  if (result != Z_OK) {
    return false;
  }
  if ((dictionary_ != NULL) && !gzip_ && !raw_) {
    result = deflateSetDictionary(&stream_, dictionary_, dictionary_length_);
    delete[] dictionary_;
    dictionary_ = NULL;
    if (result != Z_OK) {
      return false;
    }
  }
  set_initialized(true);
  return true;
}

bool ZLibDeflateFilter::Process(uint8_t* data, intptr_t length) {
  if (current_buffer_ != NULL) {
    return false;
  }
  stream_.avail_in = length;
  stream_.next_in = current_buffer_ = data;
  return true;
}

intptr_t ZLibDeflateFilter::Processed(uint8_t* buffer,
                                      intptr_t length,
                                      bool flush,
                                      bool end) {
  stream_.avail_out = length;
  stream_.next_out = buffer;
  bool error = false;
  switch (deflate(&stream_,
                  end ? Z_FINISH : flush ? Z_SYNC_FLUSH : Z_NO_FLUSH)) {
    case Z_STREAM_END:
    case Z_BUF_ERROR:
    case Z_OK: {
      intptr_t processed = length - stream_.avail_out;
      if (processed == 0) {
        break;
      }
      return processed;
    }

    default:
    case Z_STREAM_ERROR:
        error = true;
  }

  delete[] current_buffer_;
  current_buffer_ = NULL;
  // Either 0 Byte processed or error
  return error ? -1 : 0;
}

ZLibInflateFilter::~ZLibInflateFilter() {
  delete[] dictionary_;
  delete[] current_buffer_;
  if (initialized()) {
    inflateEnd(&stream_);
  }
}

bool ZLibInflateFilter::Init() {
  int window_bits = raw_ ?
      -window_bits_ :
      window_bits_ | kZLibFlagAcceptAnyHeader;

  stream_.next_in = Z_NULL;
  stream_.avail_in = 0;
  stream_.zalloc = Z_NULL;
  stream_.zfree = Z_NULL;
  stream_.opaque = Z_NULL;
  int result = inflateInit2(&stream_, window_bits);
  if (result != Z_OK) {
    return false;
  }
  set_initialized(true);
  return true;
}

bool ZLibInflateFilter::Process(uint8_t* data, intptr_t length) {
  if (current_buffer_ != NULL) {
    return false;
  }
  stream_.avail_in = length;
  stream_.next_in = current_buffer_ = data;
  return true;
}

intptr_t ZLibInflateFilter::Processed(uint8_t* buffer,
                                      intptr_t length,
                                      bool flush,
                                      bool end) {
  stream_.avail_out = length;
  stream_.next_out = buffer;
  bool error = false;
  int v;
  switch (v = inflate(&stream_,
                  end ? Z_FINISH : flush ? Z_SYNC_FLUSH : Z_NO_FLUSH)) {
    case Z_STREAM_END:
    case Z_BUF_ERROR:
    case Z_OK: {
      intptr_t processed = length - stream_.avail_out;
      if (processed == 0) {
        break;
      }
      return processed;
    }

    case Z_NEED_DICT:
      if (dictionary_ == NULL) {
        error = true;
      } else {
        int result = inflateSetDictionary(&stream_, dictionary_,
                                          dictionary_length_);
        delete[] dictionary_;
        dictionary_ = NULL;
        error = result != Z_OK;
      }
      if (error) {
        break;
      } else {
        return Processed(buffer, length, flush, end);
      }

    default:
    case Z_MEM_ERROR:
    case Z_DATA_ERROR:
    case Z_STREAM_ERROR:
      error = true;
  }

  delete[] current_buffer_;
  current_buffer_ = NULL;
  // Either 0 Byte processed or error
  return error ? -1 : 0;
}

}  // namespace dart
}  // namespace mojo
