// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/http_server/connection.h"

#include "base/bind.h"
#include "base/format_macros.h"
#include "base/strings/string_piece.h"
#include "base/strings/stringprintf.h"

namespace http_server {
namespace {

const char* GetHttpReasonPhrase(uint32_t code_in) {
  switch (code_in) {
#define HTTP_STATUS(label, code, reason) \
  case code:                             \
    return reason;
#include "services/http_server/http_status_code_list.h"
#undef HTTP_STATUS

    default:
      NOTREACHED() << "unknown HTTP status code " << code_in;
  }

  return "";
}

}  // namespace

Connection::Connection(mojo::TCPConnectedSocketPtr conn,
                       mojo::ScopedDataPipeProducerHandle sender,
                       mojo::ScopedDataPipeConsumerHandle receiver,
                       const Callback& callback)
    : connection_(conn.Pass()),
      sender_(sender.Pass()),
      receiver_(receiver.Pass()),
      request_waiter_(
          receiver_.get(),
          MOJO_HANDLE_SIGNAL_READABLE,
          base::Bind(&Connection::OnRequestDataReady, base::Unretained(this))),
      content_length_(0),
      handle_request_callback_(callback),
      response_offset_(0) {
}

Connection::~Connection() {
}

void Connection::SendResponse(HttpResponsePtr response) {
  std::string http_reason_phrase(GetHttpReasonPhrase(response->status_code));

  // TODO: should we send http/1.0 for http/1.0.requests?
  base::StringAppendF(&response_, "HTTP/1.1 %d %s\r\n", response->status_code,
                      http_reason_phrase.c_str());
  base::StringAppendF(&response_, "Connection: close\r\n");

  content_length_ = response->content_length;
  if (content_length_) {
    base::StringAppendF(&response_, "Content-Length: %" PRIuS "\r\n",
                        static_cast<size_t>(content_length_));
  }
  base::StringAppendF(&response_, "Content-Type: %s\r\n",
                      response->content_type.data());
  for (auto it = response->custom_headers.begin();
       it != response->custom_headers.end(); ++it) {
    const std::string& header_name = it.GetKey();
    const std::string& header_value = it.GetValue();
    DCHECK(header_value.find_first_of("\n\r") == std::string::npos)
        << "Malformed header value.";
    base::StringAppendF(&response_, "%s: %s\r\n", header_name.c_str(),
                        header_value.c_str());
  }
  base::StringAppendF(&response_, "\r\n");

  content_ = response->body.Pass();
  WriteMore();
}

void Connection::OnRequestDataReady(MojoResult result) {
  uint32_t num_bytes = 0;
  result =
      ReadDataRaw(receiver_.get(), NULL, &num_bytes, MOJO_READ_DATA_FLAG_QUERY);
  if (!num_bytes)
    return;

  scoped_ptr<uint8_t[]> buffer(new uint8_t[num_bytes]);
  result = ReadDataRaw(receiver_.get(), buffer.get(), &num_bytes,
                       MOJO_READ_DATA_FLAG_ALL_OR_NONE);

  request_parser_.ProcessChunk(
      base::StringPiece(reinterpret_cast<char*>(buffer.get()), num_bytes));
  if (request_parser_.ParseRequest() == HttpRequestParser::ACCEPTED) {
    handle_request_callback_.Run(this, request_parser_.GetRequest());
  }
}

void Connection::WriteMore() {
  uint32_t response_bytes_available =
      static_cast<uint32_t>(response_.size() - response_offset_);
  if (response_bytes_available) {
    MojoResult result =
        WriteDataRaw(sender_.get(), &response_[response_offset_],
                     &response_bytes_available, 0);
    if (result == MOJO_RESULT_SHOULD_WAIT) {
      sender_waiter_.reset(new mojo::AsyncWaiter(
          sender_.get(), MOJO_HANDLE_SIGNAL_WRITABLE,
          base::Bind(&Connection::OnSenderReady, base::Unretained(this))));
      return;
    } else if (result != MOJO_RESULT_OK) {
      LOG(ERROR) << "Error writing to pipe " << result;
      delete this;
      return;
    }

    response_offset_ += response_bytes_available;
  }

  if (response_offset_ != response_.size()) {
    // We have more data left in response_. Write more asynchronously.
    sender_waiter_.reset(new mojo::AsyncWaiter(
        sender_.get(), MOJO_HANDLE_SIGNAL_WRITABLE,
        base::Bind(&Connection::OnSenderReady, base::Unretained(this))));
    return;
  }

  // response_ is all sent, and there's no more data so we're done.
  if (!content_length_) {
    delete this;
    return;
  }

  // Copy data from the handler's pipe to response_.
  const uint32_t kMaxChunkSize = 1024 * 1024;

  uint32_t num_bytes_available = 0;
  MojoResult result = ReadDataRaw(content_.get(), NULL, &num_bytes_available,
                                  MOJO_READ_DATA_FLAG_QUERY);
  if (result == MOJO_RESULT_SHOULD_WAIT) {
    // Producer isn't ready yet. Wait for it.
    response_receiver_waiter_.reset(new mojo::AsyncWaiter(
        content_.get(), MOJO_HANDLE_SIGNAL_READABLE,
        base::Bind(&Connection::OnResponseDataReady, base::Unretained(this))));
    return;
  }

  DCHECK_EQ(result, MOJO_RESULT_OK);
  num_bytes_available = std::min(num_bytes_available, kMaxChunkSize);

  response_.resize(num_bytes_available);
  response_offset_ = 0;
  content_length_ -= num_bytes_available;

  result = ReadDataRaw(content_.get(), &response_[0], &num_bytes_available,
                       MOJO_READ_DATA_FLAG_ALL_OR_NONE);
  DCHECK_EQ(result, MOJO_RESULT_OK);
  sender_waiter_.reset(new mojo::AsyncWaiter(
      sender_.get(), MOJO_HANDLE_SIGNAL_WRITABLE,
      base::Bind(&Connection::OnSenderReady, base::Unretained(this))));
}

void Connection::OnResponseDataReady(MojoResult result) {
  if (result != MOJO_RESULT_OK) {
    LOG(ERROR) << "Error waiting to read data " << result;
    delete this;
    return;
  }
  WriteMore();
}

void Connection::OnSenderReady(MojoResult result) {
  if (result != MOJO_RESULT_OK) {
    LOG(ERROR) << "Error waiting to write data " << result;
    delete this;
    return;
  }
  WriteMore();
}

}  // namespace http_server
