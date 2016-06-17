// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <math.h>
#include <memory>

#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/utility/run_loop.h"
#include "mojo/services/media/audio/interfaces/audio_server.mojom.h"
#include "mojo/services/media/audio/interfaces/audio_track.mojom.h"
#include "mojo/services/media/common/cpp/circular_buffer_media_pipe_adapter.h"
#include "mojo/services/media/common/cpp/linear_transform.h"
#include "mojo/services/media/common/cpp/local_time.h"
#include "mojo/services/media/common/interfaces/timelines.mojom.h"
#include "mojo/services/media/core/interfaces/media_renderer.mojom.h"

namespace mojo {
namespace media {
namespace audio {
namespace examples {

static constexpr uint32_t SAMP_FREQ = 48000;
static constexpr uint32_t CHUNK_USEC = 10000;
static constexpr uint32_t BUF_LO_WATER_USEC = 500000;
static constexpr uint32_t BUF_HI_WATER_USEC = BUF_LO_WATER_USEC
                                            + (4 * CHUNK_USEC);
static constexpr uint32_t BUF_DEPTH_USEC    = BUF_HI_WATER_USEC
                                            + (4 * CHUNK_USEC);
static constexpr uint32_t FRAME_BYTES = sizeof(int16_t);

static inline constexpr uint32_t USecToBytes(uint64_t usec) {
  return ((usec * SAMP_FREQ) / 1000000) * FRAME_BYTES;
}

class PlayToneApp : public ApplicationImplBase {
 public:
  ~PlayToneApp() override { OnQuit(); }

  // ApplicationImplBase overrides:
  void OnInitialize() override;
  void OnQuit() override;

 private:
  void GenerateToneCbk(MediaResult res);
  void PlayTone(double freq_hz, double amplitude, double duration_sec);
  void PostShutdown();
  void Shutdown();
  void OnConnectionError(const std::string& connection_name);

  AudioServerPtr audio_server_;
  AudioTrackPtr audio_track_;
  MediaRendererPtr media_renderer_;
  TimelineConsumerPtr timeline_consumer_;
  std::unique_ptr<CircularBufferMediaPipeAdapter> audio_pipe_;

  bool     clock_started_ = false;
  uint64_t media_time_    = 0;
  double   freq_hz_       = 440.0;
  double   amplitude_     = 1.0;
  bool     shutting_down_ = false;
};

void PlayToneApp::OnQuit() {
  timeline_consumer_.reset();
  audio_pipe_.reset();
  audio_track_.reset();
  media_renderer_.reset();
  audio_server_.reset();
}

void PlayToneApp::OnInitialize() {
  mojo::ConnectToService(shell(), "mojo:audio_server",
                         GetProxy(&audio_server_));
  audio_server_.set_connection_error_handler([this]() {
    OnConnectionError("audio_server");
  });

  audio_server_->CreateTrack(
      GetProxy(&audio_track_), GetProxy(&media_renderer_));
  audio_track_.set_connection_error_handler([this]() {
    OnConnectionError("audio_track");
  });
  media_renderer_.set_connection_error_handler([this]() {
    OnConnectionError("media_renderer");
  });

  // Query the sink's format capabilities.
  Array<MediaTypeSetPtr> supported_media_types;
  auto desc_cbk = [&supported_media_types](Array<MediaTypeSetPtr> desc) {
    supported_media_types = desc.Pass();
  };
  media_renderer_->GetSupportedMediaTypes(desc_cbk);

  // TODO(johngro): this pattern is awkward.  We really don't want to be
  // calling WaitForIncomingResponse, even if we were able supply a timeout.
  // The best practice would be to defer to a handler for the message we are
  // expecting to eventually come back.
  //
  // But... what if the message never comes back?  Perhaps the service is not
  // implemented properly, or perhaps the service is malicious.  We could
  // queue a delayed message on our run loop which indicates a timeout, but
  // then what happens when when the response to Describe comes back (as
  // expected).  We don't really have a good way to cancel the "timeout"
  // message once we have queued it.  Maintaining all of the bookkeeping
  // required to nerf the callback when it happens and is discovered to be
  // useless is going to get very old, very fast.
  //
  // For now, we just do the evil thing and block during init, but I sure do
  // wish there was something nicer we could do.
  if (!media_renderer_.WaitForIncomingResponse()) {
    MOJO_LOG(ERROR)
      << "Failed to fetch sync capabilities; no response received.";
    Shutdown();
    return;
  }

  // TODO(johngro): do something useful with our capabilities description.
  supported_media_types.reset();

  // Grab the timeline consumer interface for our audio renderer.
  MediaTimelineControlSitePtr timeline_control_site;
  media_renderer_->GetTimelineControlSite(GetProxy(&timeline_control_site));
  timeline_control_site->GetTimelineConsumer(GetProxy(&timeline_consumer_));
  timeline_consumer_.set_connection_error_handler(
      [this]() { OnConnectionError("timeline_consumer"); });

  // Configure our sink for 16-bit 48KHz mono.
  AudioMediaTypeDetailsPtr pcm_cfg = AudioMediaTypeDetails::New();
  pcm_cfg->sample_format     = AudioSampleFormat::SIGNED_16;
  pcm_cfg->channels          = 1;
  pcm_cfg->frames_per_second = SAMP_FREQ;

  MediaTypePtr media_type = MediaType::New();
  media_type->medium = MediaTypeMedium::AUDIO;
  media_type->details = MediaTypeDetails::New();
  media_type->details->set_audio(pcm_cfg.Pass());
  media_type->encoding = MediaType::kAudioEncodingLpcm;

  media_renderer_->SetMediaType(media_type.Pass());
  MediaConsumerPtr pipe;
  media_renderer_->GetConsumer(GetProxy(&pipe));

  // Now that the configuration request is in-flight and we our media pipe
  // proxy, pass its interface to our circular buffer helper, set up our
  // high/low water marks, register our callback, and start to buffer our audio.
  audio_pipe_.reset(new CircularBufferMediaPipeAdapter(pipe.Pass()));
  audio_pipe_->Init(USecToBytes(BUF_DEPTH_USEC));
  audio_pipe_->SetSignalCallback(
  [this](MediaResult res) -> void {
    GenerateToneCbk(res);
  });
  audio_pipe_->SetWatermarks(USecToBytes(BUF_HI_WATER_USEC),
                             USecToBytes(BUF_LO_WATER_USEC));
}

void PlayToneApp::GenerateToneCbk(MediaResult res) {
  using MappedPacket = CircularBufferMediaPipeAdapter::MappedPacket;
  MappedPacket mapped_pkt;

  MOJO_DCHECK(freq_hz_ > 0.0);
  MOJO_DCHECK(amplitude_ >= 0.0);
  MOJO_DCHECK(amplitude_ <= 1.0);

  if (res != MediaResult::OK) {
    MOJO_LOG(ERROR) << "Fatal error in media pipe (" << res << ").";
    PostShutdown();
    return;
  }

  while (!audio_pipe_->AboveHiWater()) {
    res = audio_pipe_->CreateMediaPacket(USecToBytes(CHUNK_USEC),
                                         false,
                                         &mapped_pkt);
    if (res != MediaResult::OK) {
      MOJO_LOG(ERROR) << "Unexpected error when creating media packet ("
                 << res << ").";
      PostShutdown();
      return;
    }

    mapped_pkt.packet()->pts = media_time_;

    for (uint32_t i = 0; i < MappedPacket::kMaxRegions; ++i) {
      int16_t* data = reinterpret_cast<int16_t*>(mapped_pkt.data(i));
      uint64_t len;

      if (!data) continue;
      len = mapped_pkt.length(i);

      MOJO_DCHECK(len && !(len % FRAME_BYTES));
      len /= FRAME_BYTES;
      for (uint64_t i = 0; i < len; ++i, ++media_time_) {
        double tmp = ((M_PI * 2.0) / SAMP_FREQ) * freq_hz_ * media_time_;
        data[i] = std::numeric_limits<int16_t>::max() * amplitude_ * sin(tmp);
      }
    }

    res = audio_pipe_->SendMediaPacket(&mapped_pkt);
    if (res != MediaResult::OK) {
      MOJO_LOG(ERROR) << "Unexpected error when sending media packet ("
                 << res << ").";
      audio_pipe_->CancelMediaPacket(&mapped_pkt);
      PostShutdown();
      return;
    }
  }

  if (!clock_started_) {
    MOJO_LOG(INFO) << "Setting rate 1/1";

    timeline_consumer_->SetTimelineTransform(
        kUnspecifiedTime, 1, 1, kUnspecifiedTime, kUnspecifiedTime,
        [](bool completed) {});
    clock_started_ = true;
  }
}

void PlayToneApp::OnConnectionError(const std::string& connection_name) {
  if (!shutting_down_) {
    MOJO_LOG(ERROR) << connection_name << " connection closed unexpectedly!";
    Shutdown();
  }
}

void PlayToneApp::PostShutdown() {
  if (audio_pipe_) {
    audio_pipe_->ResetSignalCallback();
  }
  audio_pipe_ = nullptr;

  mojo::RunLoop::current()->PostDelayedTask([this]() -> void {
    Shutdown();
  }, 0);
}

void PlayToneApp::Shutdown() {
  OnQuit();
  RunLoop::current()->Quit();
}

}  // namespace examples
}  // namespace audio
}  // namespace media
}  // namespace mojo

MojoResult MojoMain(MojoHandle app_request) {
  mojo::media::audio::examples::PlayToneApp play_tone_app;
  return mojo::RunApplication(app_request, &play_tone_app);
}
