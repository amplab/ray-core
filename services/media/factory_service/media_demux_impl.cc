// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "services/media/factory_service/media_demux_impl.h"
#include "services/media/framework/parts/reader_cache.h"
#include "services/media/framework/util/callback_joiner.h"
#include "services/media/framework_mojo/mojo_reader.h"
#include "services/media/framework_mojo/mojo_type_conversions.h"

namespace mojo {
namespace media {

// static
std::shared_ptr<MediaDemuxImpl> MediaDemuxImpl::Create(
    InterfaceHandle<SeekingReader> reader,
    InterfaceRequest<MediaDemux> request,
    MediaFactoryService* owner) {
  return std::shared_ptr<MediaDemuxImpl>(
      new MediaDemuxImpl(reader.Pass(), request.Pass(), owner));
}

MediaDemuxImpl::MediaDemuxImpl(InterfaceHandle<SeekingReader> reader,
                               InterfaceRequest<MediaDemux> request,
                               MediaFactoryService* owner)
    : MediaFactoryService::Product<MediaDemux>(this, request.Pass(), owner) {
  DCHECK(reader);

  task_runner_ = base::MessageLoop::current()->task_runner();
  DCHECK(task_runner_);

  metadata_publisher_.SetCallbackRunner(
      [this](const GetMetadataCallback& callback, uint64_t version) {
        callback.Run(version, demux_ ? MediaMetadata::From(demux_->metadata())
                                     : nullptr);
      });

  std::shared_ptr<Reader> reader_ptr = MojoReader::Create(reader.Pass());
  if (!reader_ptr) {
    NOTREACHED() << "couldn't create reader";
    return;
  }

  std::shared_ptr<ReaderCache> reader_cache_ptr =
      ReaderCache::Create(reader_ptr);
  if (!reader_cache_ptr) {
    NOTREACHED() << "couldn't create reader cache";
    return;
  }

  demux_ = Demux::Create(std::shared_ptr<Reader>(reader_cache_ptr));
  if (!demux_) {
    NOTREACHED() << "couldn't create demux";
    return;
  }

  demux_->WhenInitialized([this](Result result) {
    task_runner_->PostTask(FROM_HERE,
                           base::Bind(&MediaDemuxImpl::OnDemuxInitialized,
                                      base::Unretained(this), result));
  });
}

MediaDemuxImpl::~MediaDemuxImpl() {}

void MediaDemuxImpl::OnDemuxInitialized(Result result) {
  demux_part_ = graph_.Add(demux_);

  const std::vector<Demux::DemuxStream*>& demux_streams = demux_->streams();
  for (Demux::DemuxStream* demux_stream : demux_streams) {
    streams_.push_back(std::unique_ptr<Stream>(
        new Stream(demux_part_.output(demux_stream->index()),
                   demux_stream->stream_type(), &graph_)));
  }

  graph_.Prepare();

  metadata_publisher_.SendUpdates();

  init_complete_.Occur();
}

void MediaDemuxImpl::Describe(const DescribeCallback& callback) {
  init_complete_.When([this, callback]() {
    Array<MediaTypePtr> result = Array<MediaTypePtr>::New(streams_.size());
    for (size_t i = 0; i < streams_.size(); i++) {
      MediaSourceStreamDescriptorPtr descriptor =
          MediaSourceStreamDescriptor::New();
      result[i] = streams_[i]->media_type();
    }

    callback.Run(result.Pass());
  });
}

void MediaDemuxImpl::GetProducer(uint32_t stream_index,
                                 InterfaceRequest<MediaProducer> producer) {
  RCHECK(init_complete_.occurred());

  if (stream_index >= streams_.size()) {
    return;
  }

  streams_[stream_index]->GetProducer(producer.Pass());
}

void MediaDemuxImpl::GetMetadata(uint64_t version_last_seen,
                                 const GetMetadataCallback& callback) {
  metadata_publisher_.Get(version_last_seen, callback);
}

void MediaDemuxImpl::Prime(const PrimeCallback& callback) {
  RCHECK(init_complete_.occurred());

  std::shared_ptr<CallbackJoiner> callback_joiner = CallbackJoiner::Create();

  for (std::unique_ptr<Stream>& stream : streams_) {
    stream->PrimeConnection(callback_joiner->NewCallback());
  }

  callback_joiner->WhenJoined(callback);
}

void MediaDemuxImpl::Flush(const FlushCallback& callback) {
  RCHECK(init_complete_.occurred());

  graph_.FlushAllOutputs(demux_part_);

  std::shared_ptr<CallbackJoiner> callback_joiner = CallbackJoiner::Create();

  for (std::unique_ptr<Stream>& stream : streams_) {
    stream->FlushConnection(callback_joiner->NewCallback());
  }

  callback_joiner->WhenJoined(callback);
}

void MediaDemuxImpl::Seek(int64_t position, const SeekCallback& callback) {
  RCHECK(init_complete_.occurred());

  demux_->Seek(position, [this, callback]() {
    task_runner_->PostTask(FROM_HERE, base::Bind(&RunSeekCallback, callback));
  });
}

// static
void MediaDemuxImpl::RunSeekCallback(const SeekCallback& callback) {
  callback.Run();
}

MediaDemuxImpl::Stream::Stream(OutputRef output,
                               std::unique_ptr<StreamType> stream_type,
                               Graph* graph)
    : stream_type_(std::move(stream_type)), graph_(graph), output_(output) {
  DCHECK(stream_type_);
  DCHECK(graph);

  producer_ = MojoProducer::Create();
  graph_->ConnectOutputToPart(output_, graph_->Add(producer_));
}

MediaDemuxImpl::Stream::~Stream() {}

MediaTypePtr MediaDemuxImpl::Stream::media_type() const {
  return MediaType::From(stream_type_);
}

void MediaDemuxImpl::Stream::GetProducer(
    InterfaceRequest<MediaProducer> producer) {
  DCHECK(producer_);
  producer_->AddBinding(producer.Pass());
}

void MediaDemuxImpl::Stream::PrimeConnection(
    const MojoProducer::PrimeConnectionCallback callback) {
  DCHECK(producer_);
  producer_->PrimeConnection(callback);
}

void MediaDemuxImpl::Stream::FlushConnection(
    const MojoProducer::FlushConnectionCallback callback) {
  DCHECK(producer_);
  producer_->FlushConnection(callback);
}

}  // namespace media
}  // namespace mojo
