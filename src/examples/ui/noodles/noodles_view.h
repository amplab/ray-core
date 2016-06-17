// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXAMPLES_UI_NOODLES_NOODLES_VIEW_H_
#define EXAMPLES_UI_NOODLES_NOODLES_VIEW_H_

#include <memory>
#include <mutex>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/thread.h"
#include "mojo/ui/base_view.h"
#include "third_party/skia/include/core/SkRefCnt.h"

class SkPicture;

namespace examples {

class Frame;
class Rasterizer;

class NoodlesView : public mojo::ui::BaseView {
 public:
  NoodlesView(mojo::InterfaceHandle<mojo::ApplicationConnector> app_connector,
              mojo::InterfaceRequest<mojo::ui::ViewOwner> view_owner_request);

  ~NoodlesView() override;

 private:
  // Frame queue, held by a std::shared_ptr.
  // This object acts as a shared fifo between both threads.
  class FrameQueue {
   public:
    FrameQueue();
    ~FrameQueue();

    // Puts a pending frame into the queue, drops existing frames if needed.
    // Returns true if the queue was previously empty.
    bool PutFrame(std::unique_ptr<Frame> frame);

    // Takes a pending frame from the queue.
    std::unique_ptr<Frame> TakeFrame();

   private:
    std::mutex mutex_;
    std::unique_ptr<Frame> next_frame_;  // guarded by |mutex_|

    DISALLOW_COPY_AND_ASSIGN(FrameQueue);
  };

  // Wrapper around state which is only accessible by the rasterizer thread.
  class RasterizerDelegate {
   public:
    explicit RasterizerDelegate(const std::shared_ptr<FrameQueue>& frame_queue);
    ~RasterizerDelegate();

    void CreateRasterizer(
        mojo::InterfaceHandle<mojo::ApplicationConnector> connector_info,
        mojo::InterfaceHandle<mojo::gfx::composition::Scene> scene_info);

    void PublishNextFrame();

   private:
    std::shared_ptr<FrameQueue> frame_queue_;
    std::unique_ptr<Rasterizer> rasterizer_;

    DISALLOW_COPY_AND_ASSIGN(RasterizerDelegate);
  };

  // |BaseView|:
  void OnDraw() override;

  void UpdateFrame();
  sk_sp<SkPicture> CreatePicture();

  std::shared_ptr<FrameQueue> frame_queue_;

  scoped_ptr<RasterizerDelegate> rasterizer_delegate_;  // can't use unique_ptr
                                                        // here due to
                                                        // base::Bind (sadness)
  std::unique_ptr<base::Thread> rasterizer_thread_;
  scoped_refptr<base::SingleThreadTaskRunner> rasterizer_task_runner_;

  double alpha_ = 0.0;
  int wx_ = 2;
  int wy_ = 3;

  DISALLOW_COPY_AND_ASSIGN(NoodlesView);
};

}  // namespace examples

#endif  // EXAMPLES_UI_NOODLES_NOODLES_VIEW_H_
