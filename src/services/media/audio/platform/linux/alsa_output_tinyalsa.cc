// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <tinyalsa/asoundlib.h>

#include "services/media/audio/platform/linux/alsa_output.h"

namespace mojo {
namespace media {
namespace audio {

MediaResult AlsaOutput::AlsaOpen() {
  DCHECK(output_formatter_);
  DCHECK(!alsa_device_);

  uint64_t tmp = local_time::to_msec<uint32_t>(kChunkDuration);
  tmp *= output_formatter_->format()->frames_per_second;
  tmp /= 1000;
  DCHECK(tmp <= std::numeric_limits<uint32_t>::max());

  struct pcm_config config;
  ::memset(&config, 0, sizeof(config));

  config.channels        = output_formatter_->format()->channels;
  config.rate            = output_formatter_->format()->frames_per_second;
  config.period_size     = static_cast<uint32_t>(tmp);
  config.period_count    = kTargetLatencyChunks;
  config.format          = static_cast<enum pcm_format>(alsa_format_);
  config.start_threshold = config.period_size;

  alsa_device_ = pcm_open(1, 0, PCM_OUT | PCM_NORESTART, &config);
  mix_buf_frames_ = config.period_size * config.period_count;

  return alsa_device_ ? MediaResult::OK
                      : MediaResult::INTERNAL_ERROR;
}

void AlsaOutput::AlsaClose() {
  pcm* alsa_device = static_cast<pcm*>(alsa_device_);

  if (alsa_device) {
    pcm_close(alsa_device);
    alsa_device_ = nullptr;
  }
}

MediaResult AlsaOutput::AlsaSelectFormat(
    const AudioMediaTypeDetailsPtr& config) {
  switch (config->sample_format) {
  case AudioSampleFormat::SIGNED_16:
    alsa_format_ = PCM_FORMAT_S16_LE;
    break;

  // tinyalsa does not support unsigned LPCM formats
  case AudioSampleFormat::UNSIGNED_8:
  case AudioSampleFormat::SIGNED_24_IN_32:
  default:
    return MediaResult::UNSUPPORTED_CONFIG;
  }

  return MediaResult::OK;
}

int AlsaOutput::AlsaWrite(const void* data, uint32_t frames) {
  DCHECK(alsa_device_);
  DCHECK(output_formatter_);
  pcm* alsa_device = static_cast<pcm*>(alsa_device_);

  uint32_t bytes = frames * output_formatter_->bytes_per_frame();
  int res = pcm_write(alsa_device, data, bytes);
  return !res ? frames : res;
}

int AlsaOutput::AlsaGetAvailDelay(uint32_t* avail, uint32_t* delay) {
  DCHECK(alsa_device_);
  DCHECK(avail);
  pcm* alsa_device = static_cast<pcm*>(alsa_device_);

  struct timespec tstamp;

  int res = pcm_get_htimestamp(alsa_device, avail, &tstamp);

  if (res < 0) {
    // The tinyalsa implementation tends to return -1 whenever anything goes
    // wrong, even when it has more information.  If it returns -1 from
    // pcm_get_htimestamp, and the output is in the underrun state, we actually
    // want to signal an underflow by returning -EPIPE instead of -1.
    //
    // In addition, we need to make single call to pcm_write (which will fail
    // with -EPIPE) to cause tinyalsa to recognize that it has underflowed and
    // update its bookkeeping accordingly.  The next call to write may now be
    // used to re-prime the pipeline and get things going again.
    if ((res == -1) && pcm_is_ready(alsa_device) &&
        (pcm_state(alsa_device) == PCM_STATE_XRUN)) {
      res = AlsaWrite(mix_buf_.get(), 1);
      DCHECK(res == -EPIPE);
    }
    return res;
  }

  DCHECK_LE(*avail, pcm_get_buffer_size(alsa_device));
  if (delay) {
    *delay = pcm_get_buffer_size(alsa_device) - *avail;
  }

  return 0;
}

int AlsaOutput::AlsaRecover(int err_code) {
  // tinyalsa does not seem to have a snd_pcm_recover equivalent.  If the error
  // we are attempting to recover from is underflow, and the device is actually
  // in an underflow state, then just say that we handled it and move on.
  // Otherwise, return the error which was passed in back to the caller
  // (indicating that this is an un-recoverable error).
  DCHECK(alsa_device_);
  pcm* alsa_device = static_cast<pcm*>(alsa_device_);

  if ((err_code == -EPIPE) && pcm_is_ready(alsa_device) &&
      (pcm_state(alsa_device) == PCM_STATE_XRUN)) {
    return 0;
  }

  return err_code;
}


}  // namespace audio
}  // namespace media
}  // namespace mojo
