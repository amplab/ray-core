// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_MEDIA_AUDIO_AUDIO_PIPE_H_
#define SERVICES_MEDIA_AUDIO_AUDIO_PIPE_H_

#include <memory>
#include <vector>

#include "mojo/services/media/common/cpp/linear_transform.h"
#include "services/media/audio/fwd_decls.h"
#include "services/media/common/media_pipe_base.h"

namespace mojo {
namespace media {
namespace audio {

class AudioPipe : public MediaPipeBase {
 public:
  class AudioPacketRef;
  using AudioPacketRefPtr = std::shared_ptr<AudioPacketRef>;
  using MediaPacketStatePtr = MediaPipeBase::MediaPacketStatePtr;

  class AudioPacketRef {
   public:
    struct Region {
      Region(const void* b, uint32_t ffl) : base(b), frac_frame_len(ffl) {}
      const void* base;
      uint32_t frac_frame_len;
    };

    ~AudioPacketRef();
    void SetResult(SendResult result) { return state_->SetResult(result); }

    // Accessors for starting and ending presentation time stamps expressed in
    // units of audio frames (note, not media time), as signed 51.12 fixed point
    // integers (see AudioTrackImpl:PTS_FRACTIONAL_BITS).  At 192KHz, this
    // allows for ~372.7 years of usable range when starting from a media time
    // of 0.
    //
    // AudioPackets consumed by the AudioServer are all expected to have
    // explicit presentation time stamps.  If packets sent by the user are
    // missing timestamps, appropriate timestamps will be synthesized at this
    // point in the pipeline.
    //
    // Note, the start pts is the time at which the first frame of audio in the
    // packet should be presented.  The end_pts is the time at which the frame
    // after the final frame in the packet would be presented.
    //
    // TODO(johngro): Reconsider this.  It may be best to keep things expressed
    // simply in media time instead of converting to fractional units of track
    // frames.  If/when outputs move away from a single fixed step size for
    // output sampling, it will probably be best to just convert this back to
    // media time.
    const int64_t& start_pts() const { return start_pts_; }
    const int64_t& end_pts() const { return end_pts_; }

    // Accessor for the regions in the shared buffer which contain the actual
    // sample data.
    const std::vector<Region>& regions() const { return regions_; }

    uint32_t frame_count() const { return frame_count_; }
    const MediaPacketStatePtr& state() const { return state_; }

   private:
    friend class AudioPipe;
    AudioPacketRef(MediaPacketStatePtr state,
                   AudioServerImpl* server,
                   std::vector<Region>&& regions,  // NOLINT(build/c++11)
                   int64_t start_pts,
                   int64_t end_pts,
                   uint32_t frame_count);

    MediaPacketStatePtr state_;
    AudioServerImpl* server_;

    std::vector<Region> regions_;
    int64_t start_pts_;
    int64_t end_pts_;
    uint32_t frame_count_;
  };

  AudioPipe(AudioTrackImpl* owner, AudioServerImpl* server);
  ~AudioPipe() override;

 protected:
  void OnPacketReceived(MediaPacketStatePtr state) override;
  void OnPrimeRequested(const PrimeCallback& cbk) override;
  bool OnFlushRequested(const FlushCallback& cbk) override;

 private:
  AudioTrackImpl* owner_;
  AudioServerImpl* server_;

  // State used for timestamp interpolation
  bool next_pts_known_ = 0;
  int64_t next_pts_;

  PrimeCallback prime_callback_;
};

}  // namespace audio
}  // namespace media
}  // namespace mojo

#endif  // SERVICES_MEDIA_AUDIO_AUDIO_PIPE_H_
