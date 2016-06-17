// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/wait.h"
#include "mojo/public/cpp/utility/run_loop.h"
#include "mojo/services/media/audio/interfaces/audio_server.mojom.h"
#include "mojo/services/media/audio/interfaces/audio_track.mojom.h"
#include "mojo/services/media/common/cpp/circular_buffer_media_pipe_adapter.h"
#include "mojo/services/media/common/cpp/linear_transform.h"
#include "mojo/services/media/common/cpp/local_time.h"
#include "mojo/services/media/core/interfaces/media_renderer.mojom.h"
#include "mojo/services/network/interfaces/network_service.mojom.h"
#include "mojo/services/network/interfaces/url_loader.mojom.h"

#define PACKED __attribute__((packed))

static inline constexpr uint32_t make_fourcc(uint8_t a, uint8_t b,
                                             uint8_t c, uint8_t d) {
  return (static_cast<uint32_t>(a) << 24) |
         (static_cast<uint32_t>(b) << 16) |
         (static_cast<uint32_t>(c) << 8)  |
          static_cast<uint32_t>(d);
}

static inline constexpr uint32_t fetch_fourcc(const void* source) {
  return (static_cast<uint32_t>(static_cast<const uint8_t*>(source)[0]) << 24) |
         (static_cast<uint32_t>(static_cast<const uint8_t*>(source)[1]) << 16) |
         (static_cast<uint32_t>(static_cast<const uint8_t*>(source)[2]) << 8) |
          static_cast<uint32_t>(static_cast<const uint8_t*>(source)[3]);
}

namespace mojo {
namespace media {
namespace audio {
namespace examples {

static constexpr const char* TEST_FILE =
  "http://localhost/test_content/piano2.wav";
static constexpr uint32_t BUF_DEPTH_USEC    = 500000;
static constexpr uint32_t BUF_LO_WATER_USEC = 400000;
static constexpr uint32_t BUF_HI_WATER_USEC = 450000;
static constexpr uint32_t CHUNK_SIZE_USEC   = 10000;

class PlayWAVApp : public ApplicationImplBase {
 public:
  ~PlayWAVApp() override { OnQuit(); }

  // ApplicationImplBase overrides:
  void OnInitialize() override;
  void OnQuit() override;

 private:
  using AudioPipePtr = std::unique_ptr<CircularBufferMediaPipeAdapter>;
  using AudioPacket  = CircularBufferMediaPipeAdapter::MappedPacket;
  using PacketCbk    = MediaConsumer::SendPacketCallback;

  // TODO(johngro): endianness!
  struct PACKED RIFFChunkHeader {
    uint32_t  four_cc;
    uint32_t  length;
  };

  struct PACKED WAVHeader {
    uint32_t  wave_four_cc;
    uint32_t  fmt_four_cc;
    uint32_t  fmt_chunk_len;
    uint16_t  format;
    uint16_t  channel_count;
    uint32_t  frame_rate;
    uint32_t  average_byte_rate;
    uint16_t  frame_size;
    uint16_t  bits_per_sample;
  };

  // TODO(johngro): as mentioned before... endianness!
  static constexpr uint32_t RIFF_FOUR_CC = make_fourcc('R', 'I', 'F', 'F');
  static constexpr uint32_t WAVE_FOUR_CC = make_fourcc('W', 'A', 'V', 'E');
  static constexpr uint32_t FMT_FOUR_CC  = make_fourcc('f', 'm', 't', ' ');
  static constexpr uint32_t DATA_FOUR_CC = make_fourcc('d', 'a', 't', 'a');

  static constexpr uint16_t FORMAT_LPCM  = 0x0001;
  static constexpr uint16_t FORMAT_MULAW = 0x0101;
  static constexpr uint16_t FORMAT_ALAW  = 0x0102;
  static constexpr uint16_t FORMAT_ADPCM = 0x0103;

  static const std::set<std::string> VALID_MIME_TYPES;
  static const std::set<uint16_t>    VALID_FRAME_RATES;
  static const std::set<uint16_t>    VALID_BITS_PER_SAMPLES;

  bool BlockingRead(void* buf, uint32_t len);
  void ProcessHTTPResponse(URLResponsePtr resp);

  bool ReadAndValidateRIFFHeader();
  bool ReadAndValidateWAVHeader();
  bool ReadAndValidateDATAHeader();

  void OnNeedsData(MediaResult res);
  void OnPlayoutComplete(MediaConsumer::SendResult res);
  void OnConnectionError(const std::string& connection_name);
  void PostShutdown();
  void Shutdown();

  uint32_t USecToFrames(uint32_t usec) {
    uint64_t ret = (static_cast<uint64_t>(usec) * wav_info_.frame_rate)
                 / 1000000;
    MOJO_DCHECK(ret < std::numeric_limits<uint32_t>::max());
    return ret;
  }

  uint32_t USecToBytes(uint32_t usec) {
    uint32_t frames = USecToFrames(usec);

    MOJO_DCHECK(wav_info_.frame_size);
    MOJO_DCHECK(frames <
                std::numeric_limits<uint32_t>::max() / wav_info_.frame_size);

    return frames * wav_info_.frame_size;
  }

  AudioServerPtr               audio_server_;
  AudioTrackPtr                audio_track_;
  MediaRendererPtr             media_renderer_;
  AudioPipePtr                 audio_pipe_;
  TimelineConsumerPtr          timeline_consumer_;
  AudioPacket                  audio_packet_;
  PacketCbk                    playout_complete_cbk_;
  NetworkServicePtr            network_service_;
  URLLoaderPtr                 url_loader_;
  ScopedDataPipeConsumerHandle payload_;
  uint32_t                     payload_len_;
  RIFFChunkHeader              riff_hdr_;
  WAVHeader                    wav_info_;
  RIFFChunkHeader              data_hdr_;
  bool                         sent_first_packet_ = false;
  bool                         clock_started_     = false;
  bool                         shutting_down_     = false;
};

const std::set<std::string> PlayWAVApp::VALID_MIME_TYPES({
  "audio/x-wav",
  "audio/wav",
});

const std::set<uint16_t> PlayWAVApp::VALID_FRAME_RATES({
  8000, 16000, 24000, 32000, 48000,
  11025, 22050, 44100,
});

const std::set<uint16_t> PlayWAVApp::VALID_BITS_PER_SAMPLES({
  8, 16,
});

void PlayWAVApp::OnInitialize() {
  ConnectToService(shell(), "mojo:audio_server", GetProxy(&audio_server_));
  audio_server_.set_connection_error_handler([this]() {
    OnConnectionError("audio_server");
  });

  ConnectToService(shell(), "mojo:network_service",
                   GetProxy(&network_service_));
  audio_server_.set_connection_error_handler([this]() {
    OnConnectionError("network_service");
  });

  network_service_->CreateURLLoader(GetProxy(&url_loader_));
  url_loader_.set_connection_error_handler([this]() {
    OnConnectionError("url_loader");
  });

  playout_complete_cbk_ = PacketCbk([this](MediaConsumer::SendResult res) {
                                      this->OnPlayoutComplete(res);
                                    });

  URLRequestPtr req(URLRequest::New());
  req->url    = TEST_FILE;
  req->method = "GET";

  auto cbk = [this](URLResponsePtr resp) { ProcessHTTPResponse(resp.Pass()); };
  url_loader_->Start(req.Pass(), URLLoader::StartCallback(cbk));
}

void PlayWAVApp::OnQuit() {
  if (audio_packet_.packet()) {
    MOJO_DCHECK(audio_pipe_);
    audio_pipe_->CancelMediaPacket(&audio_packet_);
  }

  payload_.reset();
  url_loader_.reset();
  network_service_.reset();
  audio_pipe_.reset();
  timeline_consumer_.reset();
  audio_track_.reset();
  media_renderer_.reset();
  audio_server_.reset();
}

bool PlayWAVApp::BlockingRead(void* buf, uint32_t op_len) {
  MojoResult res;
  uint32_t amt;

  while (true) {
    amt = op_len;
    res = ReadDataRaw(payload_.get(), buf, &amt,
                      MOJO_READ_DATA_FLAG_ALL_OR_NONE);

    if ((res == MOJO_RESULT_SHOULD_WAIT) ||
        (res == MOJO_RESULT_OUT_OF_RANGE)) {
      Wait(payload_.get(),
           MOJO_HANDLE_SIGNAL_READABLE,
           MOJO_DEADLINE_INDEFINITE,
           nullptr);
      continue;
    }

    break;
  }

  return ((res == MOJO_RESULT_OK) && (amt == op_len));
}

void PlayWAVApp::ProcessHTTPResponse(URLResponsePtr resp) {
  if (resp->mime_type.is_null() ||
     (VALID_MIME_TYPES.find(resp->mime_type) == VALID_MIME_TYPES.end())) {
    MOJO_LOG(ERROR) << "Bad MimeType \""
                    << (resp->mime_type.is_null() ? "<null>" : resp->mime_type)
                    << "\"";
    Shutdown();
    return;
  }

  payload_ = resp->body.Pass();

  if (!ReadAndValidateRIFFHeader() ||
      !ReadAndValidateWAVHeader() ||
      !ReadAndValidateDATAHeader()) {
    Shutdown();
    return;
  }

  MOJO_LOG(INFO) << "Preparing to play...";
  MOJO_LOG(INFO) << "File : " << TEST_FILE;
  MOJO_LOG(INFO) << "Rate : " << wav_info_.frame_rate;
  MOJO_LOG(INFO) << "Chan : " << wav_info_.channel_count;
  MOJO_LOG(INFO) << "BPS  : " << wav_info_.bits_per_sample;

  // Create the audio sink we will use to play this WAV file and start to
  // configure it.
  audio_server_->CreateTrack(
      GetProxy(&audio_track_), GetProxy(&media_renderer_));

  // TODO(johngro): when there is some better diagnostic information made
  // available to us, make sure that we log it so we have some way to proceed
  // with debugging.
  audio_track_.set_connection_error_handler([this]() {
    OnConnectionError("audio_track");
  });
  media_renderer_.set_connection_error_handler([this]() {
    OnConnectionError("media_renderer");
  });

  AudioMediaTypeDetailsPtr pcm_cfg = AudioMediaTypeDetails::New();
  pcm_cfg->sample_format = (wav_info_.bits_per_sample == 8)
                         ? AudioSampleFormat::UNSIGNED_8
                         : AudioSampleFormat::SIGNED_16;
  pcm_cfg->channels = wav_info_.channel_count;
  pcm_cfg->frames_per_second = wav_info_.frame_rate;

  MediaTypePtr media_type = MediaType::New();
  media_type->medium = MediaTypeMedium::AUDIO;
  media_type->details = MediaTypeDetails::New();
  media_type->details->set_audio(pcm_cfg.Pass());
  media_type->encoding = MediaType::kAudioEncodingLpcm;

  // Configure the track based on the WAV header information.
  media_renderer_->SetMediaType(media_type.Pass());
  MediaConsumerPtr media_pipe;
  media_renderer_->GetConsumer(GetProxy(&media_pipe));

  // Grab the timeline consumer interface for our audio renderer.
  MediaTimelineControlSitePtr timeline_control_site;
  media_renderer_->GetTimelineControlSite(GetProxy(&timeline_control_site));
  timeline_control_site->GetTimelineConsumer(GetProxy(&timeline_consumer_));
  timeline_consumer_.set_connection_error_handler(
      [this]() { OnConnectionError("timeline_consumer"); });

  // Set up our media pipe helper, configure its callback and water marks to
  // kick off the playback process.
  audio_pipe_.reset(new CircularBufferMediaPipeAdapter(media_pipe.Pass()));
  audio_pipe_->Init(USecToBytes(BUF_DEPTH_USEC));
  audio_pipe_->SetWatermarks(USecToBytes(BUF_HI_WATER_USEC),
                             USecToBytes(BUF_LO_WATER_USEC));
  audio_pipe_->SetSignalCallback(
  [this](MediaResult res) -> void {
    OnNeedsData(res);
  });
}

bool PlayWAVApp::ReadAndValidateRIFFHeader() {
  // Read and sanity check the top level RIFF header
  if (!BlockingRead(&riff_hdr_, sizeof(riff_hdr_))) {
    MOJO_LOG(ERROR) << "Failed to read top level RIFF header!";
    return false;
  }

  if (fetch_fourcc(&riff_hdr_.four_cc) != RIFF_FOUR_CC) {
    MOJO_LOG(ERROR) << "Missing expected 'RIFF' 4CC "
                    << "(expected 0x " << std::hex << RIFF_FOUR_CC
                    << " got 0x"       << std::hex
                    << fetch_fourcc(&riff_hdr_.four_cc)
                    << ")";
    return false;
  }

  return true;
}

bool PlayWAVApp::ReadAndValidateWAVHeader() {
  // Read the WAVE header along with its required format chunk.
  if (!BlockingRead(&wav_info_, sizeof(wav_info_))) {
    MOJO_LOG(ERROR) << "Failed to read top level WAVE header!";
    return false;
  }

  if (fetch_fourcc(&wav_info_.wave_four_cc) != WAVE_FOUR_CC) {
    MOJO_LOG(ERROR) << "Missing expected 'WAVE' 4CC "
                    << "(expected 0x " << std::hex << WAVE_FOUR_CC
                    << " got 0x"
                    << std::hex << fetch_fourcc(&wav_info_.wave_four_cc)
                    << ")";
    return false;
  }

  if (fetch_fourcc(&wav_info_.fmt_four_cc) != FMT_FOUR_CC) {
    MOJO_LOG(ERROR) << "Missing expected 'fmt ' 4CC "
                    << "(expected 0x " << std::hex << FMT_FOUR_CC
                    << " got 0x"
                    << std::hex << fetch_fourcc(&wav_info_.fmt_four_cc)
                    << ")";
    return false;
  }

  // Sanity check the format of the wave file.  This demo only support a limited
  // subset of the possible formats.
  if (wav_info_.format != FORMAT_LPCM) {
    MOJO_LOG(ERROR) << "Unsupported format (0x"
                    << std::hex << wav_info_.format
                    << ") must be LPCM (0x"
                    << std::hex << FORMAT_LPCM
                    << ")";
    return false;
  }

  if ((wav_info_.channel_count != 1) && (wav_info_.channel_count != 2)) {
    MOJO_LOG(ERROR) << "Unsupported channel count ("
                    << wav_info_.channel_count
                    << ") must be either mono or stereo";
    return false;
  }

  if (VALID_FRAME_RATES.find(wav_info_.frame_rate) == VALID_FRAME_RATES.end()) {
    MOJO_LOG(ERROR) << "Unsupported frame_rate ("
                    << wav_info_.frame_rate << ")";
    return false;
  }

  if (VALID_BITS_PER_SAMPLES.find(wav_info_.bits_per_sample) ==
      VALID_BITS_PER_SAMPLES.end()) {
    MOJO_LOG(ERROR) << "Unsupported bits per sample ("
                    << wav_info_.bits_per_sample << ")";
    return false;
  }

  uint16_t expected_frame_size;
  expected_frame_size = (wav_info_.channel_count * wav_info_.bits_per_sample)
                        >> 3;
  if (wav_info_.frame_size != expected_frame_size) {
    MOJO_LOG(ERROR) << "Frame size sanity check failed.  (expected "
                    << expected_frame_size << " got "
                    << wav_info_.frame_size << ")";
    return false;
  }

  return true;
}

bool PlayWAVApp::ReadAndValidateDATAHeader() {
  // Technically, there could be format specific member of the wave format
  // chunk, or other riff chunks which could come after this, but for this demo,
  // we only handle getting the 'data' chunk at this point.
  if (!BlockingRead(&data_hdr_, sizeof(data_hdr_))) {
    MOJO_LOG(ERROR) << "Failed to read data header!";
    return false;
  }

  if (fetch_fourcc(&data_hdr_.four_cc) != DATA_FOUR_CC) {
    MOJO_LOG(ERROR) << "Missing expected 'data' 4CC "
                    << "(expected 0x " << std::hex << DATA_FOUR_CC
                    << " got 0x"       << std::hex
                    << fetch_fourcc(&data_hdr_.four_cc)
                    << ")";
    return false;
  }

  if ((data_hdr_.length + sizeof(WAVHeader) + sizeof(RIFFChunkHeader))
      != riff_hdr_.length) {
    MOJO_LOG(ERROR) << "Header length sanity check failure ("
                    << data_hdr_.length << " + "
                    << sizeof(WAVHeader) + sizeof(RIFFChunkHeader) << " != "
                    << riff_hdr_.length << ")";
    return false;
  }

  // If the length of the data chunk is not a multiple of the frame size, log a
  // warning and truncate the length.
  uint16_t leftover;
  payload_len_ = data_hdr_.length;
  leftover     = payload_len_ % wav_info_.frame_size;
  if (leftover) {
    MOJO_LOG(WARNING)
      << "Data chunk length (" << payload_len_
      << ") not a multiple of frame size (" << wav_info_.frame_size
      << ")";
    payload_len_ -= leftover;
  }

  return true;
}

void PlayWAVApp::OnNeedsData(MediaResult res) {
  if (res != MediaResult::OK) {
    MOJO_LOG(ERROR) << "Error during playback!  (res = " << res << ")";
    PostShutdown();
    return;
  }

  if (!payload_len_) {
    // If we are just waiting for playout to finish, keep receiving callbacks so
    // we know if something went fatally wrong.
    return;
  }

  uint64_t bytes = USecToBytes(CHUNK_SIZE_USEC);
  if (bytes > payload_len_) {
    bytes = payload_len_;
  }

  res = audio_pipe_->CreateMediaPacket(bytes, false, &audio_packet_);
  if (res != MediaResult::OK) {
    MOJO_LOG(ERROR) << "Failed to create " << bytes << " byte media packet!  "
                    << "(res = " << res << ")";
    PostShutdown();
    return;
  }

  if (!sent_first_packet_) {
    MOJO_DCHECK(audio_packet_.packet());
    audio_packet_.packet()->pts = 0;
    sent_first_packet_ = true;
  }

  for (size_t i = 0; i < AudioPacket::kMaxRegions; ++i) {
    if (audio_packet_.data(i)) {
      MOJO_DCHECK(audio_packet_.length(i));
      MOJO_DCHECK(audio_packet_.length(i) <= payload_len_);

      if (!BlockingRead(audio_packet_.data(i),
                        audio_packet_.length(i))) {
        MOJO_LOG(ERROR) << "Failed to read source, shutting down...";
        PostShutdown();
        return;
      }

      payload_len_ -= audio_packet_.length(i);
    }
  }

  if (payload_len_) {
    res = audio_pipe_->SendMediaPacket(&audio_packet_);
  } else {
    res = audio_pipe_->SendMediaPacket(&audio_packet_, playout_complete_cbk_);
  }

  if (res != MediaResult::OK) {
    MOJO_LOG(ERROR) << "Failed to send media packet!  "
                    << "(res = " << res << ")";
    PostShutdown();
    return;
  }

  if (!clock_started_ && (audio_pipe_->AboveHiWater() || !payload_len_)) {
    timeline_consumer_->SetTimelineTransform(
        kUnspecifiedTime, 1, 1, kUnspecifiedTime, kUnspecifiedTime,
        [](bool completed) {});
    clock_started_ = true;
  }
}

void PlayWAVApp::OnPlayoutComplete(MediaConsumer::SendResult res) {
  MOJO_DCHECK(!audio_pipe_->GetPending());
  audio_pipe_ = nullptr;
  PostShutdown();
}

void PlayWAVApp::OnConnectionError(const std::string& connection_name) {
  if (!shutting_down_) {
    MOJO_LOG(ERROR) << connection_name << " connection closed unexpectedly!";
    PostShutdown();
  }
}

void PlayWAVApp::PostShutdown() {
  if (audio_pipe_) {
    audio_pipe_->ResetSignalCallback();
  }

  mojo::RunLoop::current()->PostDelayedTask([this]() -> void {
    Shutdown();
  }, 0);
}

// TODO(johngro): remove this when we can.  Right now, the proper way to cleanly
// shut down a running mojo application is a bit unclear to me.  Calling
// RunLoop::current()->Quit() seems like the best option, but the run loop does
// not seem to call our application's quit method.  Instead, it starts to close
// all of our connections (triggering all of our connection error handlers we
// have registered on interfaces) before finally destroying our application
// object.
//
// The net result is that we end up spurious "connection closed unexpectedly"
// error messages when we are actually shutting down cleanly.  For now, we
// suppress this by having a shutting_down_ flag and suppressing the error
// message which show up after shutdown has been triggered.  When the proper
// pattern for shutting down an app has been established, come back here and
// remove all this junk.
void PlayWAVApp::Shutdown() {
  OnQuit();
  RunLoop::current()->Quit();
}

}  // namespace examples
}  // namespace audio
}  // namespace media
}  // namespace mojo

MojoResult MojoMain(MojoHandle app_request) {
  mojo::media::audio::examples::PlayWAVApp play_wav_app;
  return mojo::RunApplication(app_request, &play_wav_app);
}
