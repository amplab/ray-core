// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "services/media/factory_service/media_decoder_impl.h"
#include "services/media/framework_mojo/mojo_type_conversions.h"

namespace mojo {
namespace media {

// static
std::shared_ptr<MediaDecoderImpl> MediaDecoderImpl::Create(
    MediaTypePtr input_media_type,
    InterfaceRequest<MediaTypeConverter> request,
    MediaFactoryService* owner) {
  return std::shared_ptr<MediaDecoderImpl>(
      new MediaDecoderImpl(input_media_type.Pass(), request.Pass(), owner));
}

MediaDecoderImpl::MediaDecoderImpl(MediaTypePtr input_media_type,
                                   InterfaceRequest<MediaTypeConverter> request,
                                   MediaFactoryService* owner)
    : MediaFactoryService::Product<MediaTypeConverter>(this,
                                                       request.Pass(),
                                                       owner),
      consumer_(MojoConsumer::Create()),
      producer_(MojoProducer::Create()) {
  DCHECK(input_media_type);

  std::unique_ptr<StreamType> input_stream_type =
      input_media_type.To<std::unique_ptr<StreamType>>();

  if (Decoder::Create(*input_stream_type, &decoder_) != Result::kOk) {
    LOG(WARNING) << "Couldn't find decoder for stream type";
    UnbindAndReleaseFromOwner();
    return;
  }

  PartRef consumer_ref = graph_.Add(consumer_);
  PartRef decoder_ref = graph_.Add(decoder_);
  PartRef producer_ref = graph_.Add(producer_);

  graph_.ConnectParts(consumer_ref, decoder_ref);
  graph_.ConnectParts(decoder_ref, producer_ref);

  consumer_->SetPrimeRequestedCallback(
      [this](const MediaConsumer::PrimeCallback& callback) {
        DCHECK(producer_);
        producer_->PrimeConnection(callback);
      });
  consumer_->SetFlushRequestedCallback(
      [this, consumer_ref](const MediaConsumer::FlushCallback& callback) {
        DCHECK(producer_);
        graph_.FlushOutput(consumer_ref.output());
        producer_->FlushConnection(callback);
      });

  graph_.Prepare();
}

MediaDecoderImpl::~MediaDecoderImpl() {}

void MediaDecoderImpl::GetOutputType(const GetOutputTypeCallback& callback) {
  DCHECK(decoder_);
  callback.Run(MediaType::From(decoder_->output_stream_type()));
}

void MediaDecoderImpl::GetConsumer(
    mojo::InterfaceRequest<MediaConsumer> consumer) {
  consumer_->AddBinding(consumer.Pass());
}

void MediaDecoderImpl::GetProducer(
    mojo::InterfaceRequest<MediaProducer> producer) {
  producer_->AddBinding(producer.Pass());
}

}  // namespace media
}  // namespace mojo
