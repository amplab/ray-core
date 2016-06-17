// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/http_server/http_request_parser.h"

#include <algorithm>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "mojo/public/cpp/system/data_pipe.h"

namespace http_server {

namespace {

size_t kRequestSizeLimit = 64 * 1024 * 1024;  // 64 mb.

// Helper function used to trim tokens in http request headers.
std::string Trim(const std::string& value) {
  std::string result;
  base::TrimString(value, " \t", &result);
  return result;
}

}  // namespace

HttpRequestParser::HttpRequestParser()
    : http_request_(HttpRequest::New()),
      buffer_position_(0),
      state_(STATE_HEADERS),
      remaining_content_bytes_(0) {
}

HttpRequestParser::~HttpRequestParser() {
}

void HttpRequestParser::ProcessChunk(const base::StringPiece& data) {
  data.AppendToString(&buffer_);
  DCHECK_LE(buffer_.size() + data.size(), kRequestSizeLimit) <<
      "The HTTP request is too large.";
}

std::string HttpRequestParser::ShiftLine() {
  size_t eoln_position = buffer_.find("\r\n", buffer_position_);
  DCHECK_NE(std::string::npos, eoln_position);
  const int line_length = eoln_position - buffer_position_;
  std::string result = buffer_.substr(buffer_position_, line_length);
  buffer_position_ += line_length + 2;
  return result;
}

HttpRequestParser::ParseResult HttpRequestParser::ParseRequest() {
  DCHECK_NE(STATE_ACCEPTED, state_);
  // Parse the request from beginning. However, entire request may not be
  // available in the buffer.
  if (state_ == STATE_HEADERS) {
    ParseResult parse_result = ParseHeaders();
    if (parse_result == ACCEPTED || parse_result == PARSE_ERROR)
      return parse_result;
  }
  // This should not be 'else if' of the previous block, as |state_| can be
  // changed in ParseHeaders().
  if (state_ == STATE_CONTENT) {
    if (ParseContent() == ACCEPTED)
      return ACCEPTED;
  }
  return WAITING;
}

HttpRequestParser::ParseResult HttpRequestParser::ParseHeaders() {
  // Check if the all request headers are available.
  if (buffer_.find("\r\n\r\n", buffer_position_) == std::string::npos)
    return WAITING;

  // Parse request's the first header line.
  // Request main main header, eg. GET /foobar.html HTTP/1.1
  {
    const std::string header_line = ShiftLine();
    std::vector<std::string> header_line_tokens = base::SplitString(
        header_line, " ", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    DCHECK_EQ(3u, header_line_tokens.size());
    // Method.
    http_request_->method = header_line_tokens[0];
    // Address.
    // Don't build an absolute URL as the parser does not know (should not
    // know) anything about the server address.
    http_request_->relative_url = header_line_tokens[1];
    // Protocol.
    const std::string protocol = base::ToLowerASCII(header_line_tokens[2]);
    CHECK(protocol == "http/1.0" || protocol == "http/1.1") <<
        "Protocol not supported: " << protocol;
  }

  // Parse further headers.
  {
    std::string header_name;
    while (true) {
      std::string header_line = ShiftLine();
      if (header_line.empty())
        break;

      if (header_line[0] == ' ' || header_line[0] == '\t') {
        // Continuation of the previous multi-line header.
        std::string header_value =
            Trim(header_line.substr(1, header_line.size() - 1));
        std::string old_value = http_request_->headers[header_name];
        http_request_->headers[header_name] = old_value + " " + header_value;
      } else {
        // New header.
        size_t delimiter_pos = header_line.find(":");
        DCHECK_NE(std::string::npos, delimiter_pos) << "Syntax error.";
        header_name = Trim(header_line.substr(0, delimiter_pos));
        std::string header_value = Trim(header_line.substr(
            delimiter_pos + 1,
            header_line.size() - delimiter_pos - 1));
        http_request_->headers[header_name] = header_value;
      }
    }
  }

  // Headers done. Is any content data attached to the request?
  size_t declared_content_length = 0;
  if (http_request_->headers.find("Content-Length") !=
      http_request_->headers.end()) {
    const bool success = base::StringToSizeT(
        http_request_->headers["Content-Length"].To<std::string>(),
        &declared_content_length);
    DCHECK(success) << "Malformed Content-Length header's value.";
  }
  if (declared_content_length == 0) {
    // No content data, so parsing is finished.
    state_ = STATE_ACCEPTED;
    return ACCEPTED;
  }

  // If we ever want to support really large content length (currently pipe max
  // 256 MB), then we'll have to stream data from parser to handler.
  MojoCreateDataPipeOptions options = {
      sizeof(MojoCreateDataPipeOptions),
      MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE, 1,
      static_cast<uint32_t>(declared_content_length)};
  MojoResult result = CreateDataPipe(
      &options, &producer_handle_, &http_request_->body);
  if (result != MOJO_RESULT_OK) {
    NOTREACHED() << "Couldn't create data pipe of size "
                 << declared_content_length;
    return PARSE_ERROR;
  }

  // The request has not yet been parsed yet, content data is still to be
  // processed.
  remaining_content_bytes_ = declared_content_length;
  state_ = STATE_CONTENT;
  return WAITING;
}

HttpRequestParser::ParseResult HttpRequestParser::ParseContent() {
  // Bytes available for read in the input buffer.
  const size_t available_bytes = buffer_.size() - buffer_position_;
  uint32_t fetch_bytes = std::min(available_bytes, remaining_content_bytes_);

  MojoResult result = WriteDataRaw(
      producer_handle_.get(),
      buffer_.data() + buffer_position_,
      &fetch_bytes,
      MOJO_WRITE_DATA_FLAG_ALL_OR_NONE);
  DCHECK_EQ(result, MOJO_RESULT_OK);
  buffer_position_ += fetch_bytes;
  remaining_content_bytes_ -= fetch_bytes;

  if (remaining_content_bytes_ == 0) {
    state_ = STATE_ACCEPTED;
    producer_handle_.reset();
    return ACCEPTED;
  }

  state_ = STATE_CONTENT;
  return WAITING;
}

HttpRequestPtr HttpRequestParser::GetRequest() {
  DCHECK_EQ(STATE_ACCEPTED, state_);
  return http_request_.Pass();
}

}  // namespace http_server
