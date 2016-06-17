// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <alsa/asoundlib.h>

#include "services/media/audio/platform/linux/alsa_output.h"

namespace mojo {
namespace media {
namespace audio {

MediaResult AlsaOutput::AlsaOpen() {
  DCHECK(output_formatter_);
  DCHECK(!alsa_device_);

  snd_pcm_sframes_t res;
  snd_pcm_t* alsa_device;
  static const char* kAlsaDevice = "default";
  res = snd_pcm_open(&alsa_device,
                     kAlsaDevice,
                     SND_PCM_STREAM_PLAYBACK,
                     SND_PCM_NONBLOCK);
  alsa_device_ = alsa_device;
  if (res != 0) {
    LOG(ERROR) << "Failed to open ALSA device \"" << kAlsaDevice << "\".";
    return MediaResult::INTERNAL_ERROR;
  }
  DCHECK(alsa_device_);
  DCHECK(alsa_device_ == alsa_device);

  res = snd_pcm_set_params(alsa_device,
                           static_cast<snd_pcm_format_t>(alsa_format_),
                           SND_PCM_ACCESS_RW_INTERLEAVED,
                           output_formatter_->format()->channels,
                           output_formatter_->format()->frames_per_second,
                           0,   // do not allow ALSA resample
                           local_time::to_usec<unsigned int>(kTargetLatency));
  if (res) {
    LOG(ERROR) << "Failed to configure ALSA device \"" << kAlsaDevice << "\" "
               << "(res = " << res << ")";
    LOG(ERROR) << "Requested channels         : "
               << output_formatter_->format()->channels;
    LOG(ERROR) << "Requested frames per second: "
               << output_formatter_->format()->frames_per_second;
    LOG(ERROR) << "Requested ALSA format      : " << alsa_format_;
    return MediaResult::INTERNAL_ERROR;
  }

  // Figure out how big our mixing buffer needs to be, then allocate it.
  res = snd_pcm_avail_update(alsa_device);
  if (res <= 0) {
    LOG(ERROR) << "[" << this << "] : "
               << "Fatal error (" << res
               << ") attempting to determine ALSA buffer size.";
    return MediaResult::INTERNAL_ERROR;
  }

  mix_buf_frames_ = res;
  return MediaResult::OK;
}

void AlsaOutput::AlsaClose() {
  snd_pcm_t* alsa_device = static_cast<snd_pcm_t*>(alsa_device_);

  if (alsa_device) {
    snd_pcm_close(alsa_device);
    alsa_device_ = nullptr;
  }
}

MediaResult AlsaOutput::AlsaSelectFormat(
    const AudioMediaTypeDetailsPtr& config) {
  switch (config->sample_format) {
  case AudioSampleFormat::UNSIGNED_8:
    alsa_format_ = SND_PCM_FORMAT_U8;
    break;

  case AudioSampleFormat::SIGNED_16:
    alsa_format_ = SND_PCM_FORMAT_S16;
    break;

  case AudioSampleFormat::SIGNED_24_IN_32:
  default:
    return MediaResult::UNSUPPORTED_CONFIG;
  }

  return MediaResult::OK;
}

int AlsaOutput::AlsaWrite(const void* data, uint32_t frames) {
  DCHECK(alsa_device_);
  snd_pcm_t* alsa_device = static_cast<snd_pcm_t*>(alsa_device_);
  return snd_pcm_writei(alsa_device, data, frames);
}

int AlsaOutput::AlsaGetAvailDelay(uint32_t* avail, uint32_t* delay) {
  DCHECK(alsa_device_);
  DCHECK(avail);
  snd_pcm_t* alsa_device = static_cast<snd_pcm_t*>(alsa_device_);

  int res;
  if (delay) {
    snd_pcm_sframes_t avail_sframes, delay_sframes;
    res = snd_pcm_avail_delay(alsa_device, &avail_sframes, &delay_sframes);
    DCHECK((res < 0) || (avail_sframes >= 0));
    DCHECK((res < 0) || (delay_sframes >= 0));
    *avail = static_cast<uint32_t>(avail_sframes);
    *delay = static_cast<uint32_t>(delay_sframes);
  } else {
    res = snd_pcm_avail_update(alsa_device);

    if (res < 0) {
      *avail = 0;
    } else {
      *avail = res;
      res = 0;
    }
  }

  return res;
}

int AlsaOutput::AlsaRecover(int err_code) {
  DCHECK(alsa_device_);
  snd_pcm_t* alsa_device = static_cast<snd_pcm_t*>(alsa_device_);
  return snd_pcm_recover(alsa_device, err_code, true);
}

}  // namespace audio
}  // namespace media
}  // namespace mojo
