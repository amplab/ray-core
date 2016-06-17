// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_SERVICES_MEDIA_FACTORY_MEDIA_DEMUX_IMPL_H_
#define MOJO_SERVICES_MEDIA_FACTORY_MEDIA_DEMUX_IMPL_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "base/single_thread_task_runner.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/services/media/core/interfaces/media_demux.mojom.h"
#include "mojo/services/media/core/interfaces/seeking_reader.mojom.h"
#include "services/media/common/mojo_publisher.h"
#include "services/media/factory_service/factory_service.h"
#include "services/media/framework/graph.h"
#include "services/media/framework/parts/demux.h"
#include "services/media/framework_mojo/mojo_producer.h"
#include "services/util/cpp/incident.h"

namespace mojo {
namespace media {

// Mojo agent that decodes a stream.
class MediaDemuxImpl : public MediaFactoryService::Product<MediaDemux>,
                       public MediaDemux {
 public:
  static std::shared_ptr<MediaDemuxImpl> Create(
      InterfaceHandle<SeekingReader> reader,
      InterfaceRequest<MediaDemux> request,
      MediaFactoryService* owner);

  ~MediaDemuxImpl() override;

  // MediaDemux implementation.
  void Describe(const DescribeCallback& callback) override;

  void GetProducer(uint32_t stream_index,
                   InterfaceRequest<MediaProducer> producer) override;

  void GetMetadata(uint64_t version_last_seen,
                   const GetMetadataCallback& callback) override;

  void Prime(const PrimeCallback& callback) override;

  void Flush(const FlushCallback& callback) override;

  void Seek(int64_t position, const SeekCallback& callback) override;

 private:
  MediaDemuxImpl(InterfaceHandle<SeekingReader> reader,
                 InterfaceRequest<MediaDemux> request,
                 MediaFactoryService* owner);

  class Stream {
   public:
    Stream(OutputRef output,
           std::unique_ptr<StreamType> stream_type,
           Graph* graph);

    ~Stream();

    // Gets the media type of the stream.
    MediaTypePtr media_type() const;

    // Gets the producer.
    void GetProducer(InterfaceRequest<MediaProducer> producer);

    // Tells the producer to prime its connection.
    void PrimeConnection(const MojoProducer::PrimeConnectionCallback callback);

    // Tells the producer to flush its connection.
    void FlushConnection(const MojoProducer::FlushConnectionCallback callback);

   private:
    std::unique_ptr<StreamType> stream_type_;
    Graph* graph_;
    OutputRef output_;
    std::shared_ptr<MojoProducer> producer_;
  };

  // Handles the completion of demux initialization.
  void OnDemuxInitialized(Result result);

  static void RunSeekCallback(const SeekCallback& callback);

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  Graph graph_;
  PartRef demux_part_;
  std::shared_ptr<Demux> demux_;
  Incident init_complete_;
  std::vector<std::unique_ptr<Stream>> streams_;
  MojoPublisher<GetMetadataCallback> metadata_publisher_;
};

}  // namespace media
}  // namespace mojo

#endif  // MOJO_SERVICES_MEDIA_FACTORY_MEDIA_DEMUX_IMPL_H_
