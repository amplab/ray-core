// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/media/framework_mojo/mojo_formatting.h"

namespace mojo {
namespace media {

template <typename T>
std::ostream& operator<<(std::ostream& os, const InterfacePtr<T>& value) {
  if (!value.is_bound()) {
    return os << "<not bound>" << std::endl;
  } else {
    return os << "<bound>" << std::endl;
  }
}

std::ostream& operator<<(std::ostream& os,
                         const MediaSourceStreamDescriptorPtr& value) {
  if (!value) {
    return os << "<nullptr>" << std::endl;
  } else {
    os << std::endl;
  }

  os << indent;
  os << begl << "uint32_t index: " << int(value->index) << std::endl;
  os << begl << "MediaTypePtr media_type: " << value->media_type;
  os << begl
     << "MediaTypePtr original_media_type: " << value->original_media_type;
  return os << outdent;
}

std::ostream& operator<<(std::ostream& os, const MediaTypePtr& value) {
  if (!value) {
    return os << "<nullptr>" << std::endl;
  } else {
    os << std::endl;
  }

  os << indent;
  os << begl
     << "MediaTypeMedium medium: " << StringFromMediaTypeMedium(value->medium)
     << std::endl;
  os << begl << "MediaTypeDetailsPtr details: " << value->details;
  os << begl << "string encoding: " << value->encoding << std::endl;
  if (value->encoding_parameters) {
    os << begl << "array<uint8>? encoding_parameters: "
       << value->encoding_parameters.size() << " bytes" << std::endl;
  } else {
    os << begl << "array<uint8>? encoding_parameters: <nullptr>" << std::endl;
  }
  return os << outdent;
}

std::ostream& operator<<(std::ostream& os, const MediaTypeSetPtr& value) {
  if (!value) {
    return os << "<nullptr>" << std::endl;
  } else {
    os << std::endl;
  }

  os << indent;
  os << begl
     << "MediaTypeMedium medium: " << StringFromMediaTypeMedium(value->medium)
     << std::endl;
  os << begl << "MediaTypeSetDetailsPtr details: " << value->details;
  os << begl << "array<string> encodings: " << value->encodings;
  return os << outdent;
}

std::ostream& operator<<(std::ostream& os, const MediaTypeDetailsPtr& value) {
  if (!value) {
    return os << "<nullptr>" << std::endl;
  } else if (value->has_unknown_tag()) {
    return os << "<empty>" << std::endl;
  } else {
    os << std::endl;
  }

  os << indent;
  if (value->is_audio()) {
    return os << begl
              << "AudioMediaTypeDetailsPtr* audio: " << value->get_audio()
              << outdent;
  }
  if (value->is_video()) {
    return os << begl
              << "VideoMediaTypeDetailsPtr* video: " << value->get_video()
              << outdent;
  }
  if (value->is_text()) {
    return os << begl << "TextMediaTypeDetailsPtr* text: " << value->get_text()
              << outdent;
  }
  if (value->is_subpicture()) {
    return os << begl << "SubpictureMediaTypeDetailsPtr* video: "
              << value->get_subpicture() << outdent;
  }
  return os << begl << "UNKNOWN TAG" << std::endl << outdent;
}

std::ostream& operator<<(std::ostream& os,
                         const MediaTypeSetDetailsPtr& value) {
  if (!value) {
    return os << "<nullptr>" << std::endl;
  } else if (value->has_unknown_tag()) {
    return os << "<empty>" << std::endl;
  } else {
    os << std::endl;
  }

  os << indent;
  if (value->is_audio()) {
    return os << begl
              << "AudioMediaTypeSetDetailsPtr* audio: " << value->get_audio()
              << outdent;
  }
  if (value->is_video()) {
    return os << begl
              << "VideoMediaTypeSetDetailsPtr* video: " << value->get_video()
              << outdent;
  }
  if (value->is_text()) {
    return os << begl
              << "TextMediaTypeSetDetailsPtr* video: " << value->get_text()
              << outdent;
  }
  if (value->is_subpicture()) {
    return os << begl << "SubpictureMediaTypeSetDetailsPtr* video: "
              << value->get_subpicture() << outdent;
  }
  return os << begl << "UNKNOWN TAG" << std::endl << outdent;
}

std::ostream& operator<<(std::ostream& os,
                         const AudioMediaTypeDetailsPtr& value) {
  if (!value) {
    return os << "<nullptr>" << std::endl;
  } else {
    os << std::endl;
  }

  os << indent;
  os << begl << "AudioSampleFormat sample_format: "
     << StringFromAudioSampleFormat(value->sample_format) << std::endl;
  os << begl << "uint32_t channels: " << int(value->channels) << std::endl;
  os << begl << "uint32_t frames_per_second: " << value->frames_per_second
     << std::endl;
  return os << outdent;
}

std::ostream& operator<<(std::ostream& os,
                         const AudioMediaTypeSetDetailsPtr& value) {
  if (!value) {
    return os << "<nullptr>" << std::endl;
  } else {
    os << std::endl;
  }

  os << indent;
  os << begl << "AudioSampleFormat sample_format: "
     << StringFromAudioSampleFormat(value->sample_format) << std::endl;
  os << begl << "uint32_t min_channels: " << int(value->min_channels)
     << std::endl;
  os << begl << "uint32_t max_channels: " << int(value->max_channels)
     << std::endl;
  os << begl
     << "uint32_t min_frames_per_second: " << value->min_frames_per_second
     << std::endl;
  os << begl
     << "uint32_t max_cframes_per_second: " << value->max_frames_per_second
     << std::endl;
  return os << outdent;
}

std::ostream& operator<<(std::ostream& os,
                         const VideoMediaTypeDetailsPtr& value) {
  if (!value) {
    return os << "<nullptr>" << std::endl;
  } else {
    os << std::endl;
  }

  os << indent;
  os << begl << "VideoProfile profile: " << value->profile << std::endl;
  os << begl << "PixelFormat pixel_format: " << value->pixel_format
     << std::endl;
  os << begl << "ColorSpace color_space: " << value->color_space << std::endl;
  os << begl << "uint32_t width: " << value->width << std::endl;
  os << begl << "uint32_t height: " << value->height << std::endl;
  os << begl << "uint32_t coded_width: " << value->coded_width << std::endl;
  os << begl << "uint32_t coded_height: " << value->coded_height << std::endl;
  return os << outdent;
}

std::ostream& operator<<(std::ostream& os,
                         const VideoMediaTypeSetDetailsPtr& value) {
  if (!value) {
    return os << "<nullptr>" << std::endl;
  } else {
    os << std::endl;
  }

  os << indent;
  os << begl << "uint32_t min_width: " << value->min_width << std::endl;
  os << begl << "uint32_t max_width: " << value->max_width << std::endl;
  os << begl << "uint32_t min_height: " << value->min_height << std::endl;
  os << begl << "uint32_t max_height: " << value->max_height << std::endl;
  return os << outdent;
}

std::ostream& operator<<(std::ostream& os,
                         const TextMediaTypeDetailsPtr& value) {
  if (!value) {
    return os << "<nullptr>" << std::endl;
  } else {
    os << std::endl;
  }

  os << indent;
  return os << outdent;
}

std::ostream& operator<<(std::ostream& os,
                         const TextMediaTypeSetDetailsPtr& value) {
  if (!value) {
    return os << "<nullptr>" << std::endl;
  } else {
    os << std::endl;
  }

  os << indent;
  return os << outdent;
}

std::ostream& operator<<(std::ostream& os,
                         const SubpictureMediaTypeDetailsPtr& value) {
  if (!value) {
    return os << "<nullptr>" << std::endl;
  } else {
    os << std::endl;
  }

  os << indent;
  return os << outdent;
}

std::ostream& operator<<(std::ostream& os,
                         const SubpictureMediaTypeSetDetailsPtr& value) {
  if (!value) {
    return os << "<nullptr>" << std::endl;
  } else {
    os << std::endl;
  }

  os << indent;
  return os << outdent;
}

std::ostream& operator<<(std::ostream& os, const TimelineTransformPtr& value) {
  if (!value) {
    return os << "<nullptr>" << std::endl;
  } else {
    os << std::endl;
  }

  os << indent;
  os << begl << "int64 reference_time: " << value->reference_time
     << std::endl;
  os << begl << "int64 subject_time: " << value->subject_time << std::endl;
  os << begl << "uint32 reference_delta: " << value->reference_delta
     << std::endl;
  os << begl << "uint32 subject_delta: " << value->subject_delta << std::endl;
  return os << outdent;
}

std::ostream& operator<<(std::ostream& os, const HttpHeaderPtr& value) {
  if (!value) {
    return os << "<nullptr>" << std::endl;
  } else {
    return os << value->name << ":" << value->value << std::endl;
  }
}

std::ostream& operator<<(std::ostream& os, const URLRequestPtr& value) {
  if (!value) {
    return os << "<nullptr>" << std::endl;
  } else {
    os << std::endl;
  }

  os << indent;
  os << begl << "mojo::String url: " << value->url << std::endl;
  os << begl << "mojo::String method: " << value->method << std::endl;
  os << begl << "mojo::Array<mojo::HttpHeaderPtr> headers: " << value->headers;
  os << begl
     << "mojo::Array<mojo::ScopedDataPipeConsumerHandle> body: " << value->body;
  os << begl << "uint32_t response_body_buffer_size: "
     << value->response_body_buffer_size << std::endl;
  os << begl << "bool auto_follow_redirects: " << value->auto_follow_redirects
     << std::endl;
  os << begl << "URLRequest::CacheMode cache_mode: " << value->cache_mode
     << std::endl;
  return os << outdent;
}

std::ostream& operator<<(std::ostream& os, const URLResponsePtr& value) {
  if (!value) {
    return os << "<nullptr>" << std::endl;
  } else {
    os << std::endl;
  }

  os << indent;
  os << begl << "mojo::NetworkErrorPtr error: " << value->error;
  os << begl << "mojo::ScopedDataPipeConsumerHandle body: " << value->body
     << std::endl;
  os << begl << "mojo::String url: " << value->url << std::endl;
  os << begl << "uint32_t status_code: " << value->status_code << std::endl;
  os << begl << "mojo::String status_line: " << value->status_line << std::endl;
  os << begl << "mojo::Array<mojo::HttpHeaderPtr> headers: " << value->headers;
  os << begl << "mojo::String mime_type: " << value->mime_type << std::endl;
  os << begl << "mojo::String charset: " << value->charset << std::endl;
  os << begl << "mojo::String redirect_method: " << value->redirect_method
     << std::endl;
  os << begl << "mojo::String redirect_url: " << value->redirect_url
     << std::endl;
  os << begl << "mojo::String redirect_referrer: " << value->redirect_referrer
     << std::endl;
  return os << outdent;
}

std::ostream& operator<<(std::ostream& os, const NetworkErrorPtr& value) {
  if (!value) {
    return os << "<nullptr>" << std::endl;
  } else {
    os << std::endl;
  }

  os << indent;
  os << begl << "int32_t code: " << value->code << std::endl;
  os << begl << "mojo::String description: " << value->description << std::endl;
  return os << outdent;
}

std::ostream& operator<<(std::ostream& os,
                         const ScopedDataPipeConsumerHandle& value) {
  if (value.is_valid()) {
    return os << "<valid>";
  } else {
    return os << "<not valid>";
  }
}

const char* StringFromMediaTypeMedium(MediaTypeMedium value) {
  switch (value) {
    case MediaTypeMedium::AUDIO:
      return "AUDIO";
    case MediaTypeMedium::VIDEO:
      return "VIDEO";
    case MediaTypeMedium::TEXT:
      return "TEXT";
    case MediaTypeMedium::SUBPICTURE:
      return "SUBPICTURE";
  }
  return "UNKNOWN MEDIUM";
}

const char* StringFromAudioSampleFormat(AudioSampleFormat value) {
  switch (value) {
    case AudioSampleFormat::ANY:
      return "ANY";
    case AudioSampleFormat::UNSIGNED_8:
      return "UNSIGNED_8";
    case AudioSampleFormat::SIGNED_16:
      return "SIGNED_16";
    case AudioSampleFormat::SIGNED_24_IN_32:
      return "SIGNED_24_IN_32";
    case AudioSampleFormat::FLOAT:
      return "FLOAT";
  }
  return "UNKNOWN FORMAT";
}

}  // namespace media
}  // namespace mojo
