// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_SERVICES_MEDIA_FACTORY_MEDIA_DECODER_IMPL_H_
#define MOJO_SERVICES_MEDIA_FACTORY_MEDIA_DECODER_IMPL_H_

#include <memory>

#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/services/media/core/interfaces/media_type_converter.mojom.h"
#include "services/media/factory_service/factory_service.h"
#include "services/media/framework/graph.h"
#include "services/media/framework/parts/decoder.h"
#include "services/media/framework_mojo/mojo_consumer.h"
#include "services/media/framework_mojo/mojo_producer.h"

namespace mojo {
namespace media {

// Mojo agent that decodes a stream.
class MediaDecoderImpl
    : public MediaFactoryService::Product<MediaTypeConverter>,
      public MediaTypeConverter {
 public:
  static std::shared_ptr<MediaDecoderImpl> Create(
      MediaTypePtr input_media_type,
      InterfaceRequest<MediaTypeConverter> request,
      MediaFactoryService* owner);

  ~MediaDecoderImpl() override;

  // MediaTypeConverter implementation.
  void GetOutputType(const GetOutputTypeCallback& callback) override;

  void GetConsumer(InterfaceRequest<MediaConsumer> consumer) override;

  void GetProducer(InterfaceRequest<MediaProducer> producer) override;

 private:
  MediaDecoderImpl(MediaTypePtr input_media_type,
                   InterfaceRequest<MediaTypeConverter> request,
                   MediaFactoryService* owner);

  Graph graph_;
  std::shared_ptr<MojoConsumer> consumer_;
  std::shared_ptr<Decoder> decoder_;
  std::shared_ptr<MojoProducer> producer_;
};

}  // namespace media
}  // namespace mojo

#endif  // MOJO_SERVICES_MEDIA_FACTORY_MEDIA_DECODER_IMPL_H_
