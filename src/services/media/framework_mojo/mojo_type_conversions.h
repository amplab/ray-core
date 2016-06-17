// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_MEDIA_FRAMEWORK_MOJO_MOJO_TYPE_CONVERSIONS_H_
#define SERVICES_MEDIA_FRAMEWORK_MOJO_MOJO_TYPE_CONVERSIONS_H_

#include "mojo/services/media/common/interfaces/media_common.mojom.h"
#include "mojo/services/media/common/interfaces/media_metadata.mojom.h"
#include "mojo/services/media/common/interfaces/media_types.mojom.h"
#include "services/media/framework/metadata.h"
#include "services/media/framework/result.h"
#include "services/media/framework/types/audio_stream_type.h"
#include "services/media/framework/types/stream_type.h"
#include "services/media/framework/types/video_stream_type.h"

namespace mojo {
namespace media {

// Converts a MojoResult into a Result.
Result ConvertResult(MojoResult mojo_result);

// Converts a MediaResult into a Result.
Result Convert(MediaResult media_result);

// Creates a StreamType::Medium from a MediaTypeMedium.
StreamType::Medium Convert(MediaTypeMedium media_type_medium);

// Creates an AudioStreamType::SampleFormat from an AudioSampleFormat.
AudioStreamType::SampleFormat Convert(AudioSampleFormat audio_sample_format);

// Creates a VideoStreamType::VideoProfile from a VideoProfile.
VideoStreamType::VideoProfile Convert(VideoProfile video_profile);

// Creates a VideoStreamType::PixelFormat from a PixelFormat.
VideoStreamType::PixelFormat Convert(PixelFormat pixel_format);

// Creates a VideoStreamType::ColorSpace from a ColorSpace.
VideoStreamType::ColorSpace Convert(ColorSpace color_space);

// Creates a MediaTypeMedium from a StreamType::Medium.
MediaTypeMedium Convert(StreamType::Medium medium);

// Creates an AudioSampleFormat from an AudioStreamType::SampleFormat.
AudioSampleFormat Convert(AudioStreamType::SampleFormat sample_format);

// Creates a VideoProfile from a VideoStreamType::VideoProfile.
VideoProfile Convert(VideoStreamType::VideoProfile video_profile);

// Creates a PixelFormat from a VideoStreamType::PixelFormat.
PixelFormat Convert(VideoStreamType::PixelFormat pixel_format);

// Creates a ColorSpace from a VideoStreamType::ColorSpace.
ColorSpace Convert(VideoStreamType::ColorSpace color_space);

}  // namespace media

template <>
struct TypeConverter<media::MediaTypePtr, std::unique_ptr<media::StreamType>> {
  static media::MediaTypePtr Convert(
      const std::unique_ptr<media::StreamType>& input);
};

template <>
struct TypeConverter<std::unique_ptr<media::StreamType>, media::MediaTypePtr> {
  static std::unique_ptr<media::StreamType> Convert(
      const media::MediaTypePtr& input);
};

template <>
struct TypeConverter<media::MediaTypeSetPtr,
                     std::unique_ptr<media::StreamTypeSet>> {
  static media::MediaTypeSetPtr Convert(
      const std::unique_ptr<media::StreamTypeSet>& input);
};

template <>
struct TypeConverter<std::unique_ptr<media::StreamTypeSet>,
                     media::MediaTypeSetPtr> {
  static std::unique_ptr<media::StreamTypeSet> Convert(
      const media::MediaTypeSetPtr& input);
};

template <>
struct TypeConverter<media::MediaMetadataPtr,
                     std::unique_ptr<media::Metadata>> {
  static media::MediaMetadataPtr Convert(
      const std::unique_ptr<media::Metadata>& input);
};

template <>
struct TypeConverter<std::unique_ptr<media::Metadata>,
                     media::MediaMetadataPtr> {
  static std::unique_ptr<media::Metadata> Convert(
      const media::MediaMetadataPtr& input);
};

template <>
struct TypeConverter<
    Array<media::MediaTypePtr>,
    std::unique_ptr<std::vector<std::unique_ptr<media::StreamType>>>> {
  static Array<media::MediaTypePtr> Convert(
      const std::unique_ptr<std::vector<std::unique_ptr<media::StreamType>>>&
          input);
};

template <>
struct TypeConverter<
    std::unique_ptr<std::vector<std::unique_ptr<media::StreamType>>>,
    Array<media::MediaTypePtr>> {
  static std::unique_ptr<std::vector<std::unique_ptr<media::StreamType>>>
  Convert(const Array<media::MediaTypePtr>& input);
};

template <>
struct TypeConverter<
    Array<media::MediaTypeSetPtr>,
    std::unique_ptr<std::vector<std::unique_ptr<media::StreamTypeSet>>>> {
  static Array<media::MediaTypeSetPtr> Convert(
      const std::unique_ptr<std::vector<std::unique_ptr<media::StreamTypeSet>>>&
          input);
};

template <>
struct TypeConverter<
    std::unique_ptr<std::vector<std::unique_ptr<media::StreamTypeSet>>>,
    Array<media::MediaTypeSetPtr>> {
  static std::unique_ptr<std::vector<std::unique_ptr<media::StreamTypeSet>>>
  Convert(const Array<media::MediaTypeSetPtr>& input);
};

template <>
struct TypeConverter<Array<uint8_t>, std::unique_ptr<media::Bytes>> {
  static Array<uint8_t> Convert(const std::unique_ptr<media::Bytes>& input);
};

template <>
struct TypeConverter<std::unique_ptr<media::Bytes>, Array<uint8_t>> {
  static std::unique_ptr<media::Bytes> Convert(const Array<uint8_t>& input);
};

}  // namespace mojo

#endif  // SERVICES_MEDIA_FRAMEWORK_MOJO_MOJO_TYPE_CONVERSIONS_H_
