// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "services/media/framework/types/audio_stream_type.h"
#include "services/media/framework/util/safe_clone.h"

namespace mojo {
namespace media {

AudioStreamType::AudioStreamType(const std::string& encoding,
                                 std::unique_ptr<Bytes> encoding_parameters,
                                 SampleFormat sample_format,
                                 uint32_t channels,
                                 uint32_t frames_per_second)
    : StreamType(StreamType::Medium::kAudio,
                 encoding,
                 std::move(encoding_parameters)),
      sample_format_(sample_format),
      channels_(channels),
      frames_per_second_(frames_per_second),
      sample_size_(SampleSizeFromFormat(sample_format)) {}

AudioStreamType::AudioStreamType(const AudioStreamType& other)
    : AudioStreamType(other.encoding(),
                      SafeClone(other.encoding_parameters()),
                      other.sample_format(),
                      other.channels(),
                      other.frames_per_second()) {}

AudioStreamType::~AudioStreamType() {}

const AudioStreamType* AudioStreamType::audio() const {
  return this;
}

// static
uint32_t AudioStreamType::SampleSizeFromFormat(SampleFormat sample_format) {
  switch (sample_format) {
    case SampleFormat::kAny:
      LOG(ERROR) << "sample size requested for SampleFormat::kAny";
      abort();
    case SampleFormat::kUnsigned8:
      return sizeof(uint8_t);
    case SampleFormat::kSigned16:
      return sizeof(int16_t);
    case SampleFormat::kSigned24In32:
      return sizeof(int32_t);
    case SampleFormat::kFloat:
      return sizeof(float);
  }

  return 0;
}

std::unique_ptr<StreamType> AudioStreamType::Clone() const {
  return Create(encoding(), SafeClone(encoding_parameters()), sample_format(),
                channels(), frames_per_second());
}

AudioStreamTypeSet::AudioStreamTypeSet(
    const std::vector<std::string>& encodings,
    AudioStreamType::SampleFormat sample_format,
    Range<uint32_t> channels,
    Range<uint32_t> frames_per_second)
    : StreamTypeSet(StreamType::Medium::kAudio, encodings),
      sample_format_(sample_format),
      channels_(channels),
      frames_per_second_(frames_per_second) {}

AudioStreamTypeSet::~AudioStreamTypeSet() {}

const AudioStreamTypeSet* AudioStreamTypeSet::audio() const {
  return this;
}

bool AudioStreamTypeSet::contains(const AudioStreamType& type) const {
  return (sample_format() == type.sample_format() ||
          sample_format() == AudioStreamType::SampleFormat::kAny) &&
         channels().contains(type.frames_per_second()) &&
         frames_per_second().contains(type.frames_per_second());
}

std::unique_ptr<StreamTypeSet> AudioStreamTypeSet::Clone() const {
  return Create(encodings(), sample_format(), channels(), frames_per_second());
}

}  // namespace media
}  // namespace mojo
