// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_GFX_COMPOSITOR_BACKEND_GPU_OUTPUT_H_
#define SERVICES_GFX_COMPOSITOR_BACKEND_GPU_OUTPUT_H_

#include <memory>
#include <mutex>
#include <queue>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/synchronization/waitable_event.h"
#include "base/task_runner.h"
#include "base/threading/thread.h"
#include "mojo/services/gpu/interfaces/context_provider.mojom.h"
#include "services/gfx/compositor/backend/gpu_rasterizer.h"
#include "services/gfx/compositor/backend/output.h"
#include "services/gfx/compositor/backend/vsync_scheduler.h"

namespace compositor {

// Renderer backed by a ContextProvider.
class GpuOutput : public Output, private GpuRasterizer::Callbacks {
 public:
  GpuOutput(mojo::InterfaceHandle<mojo::ContextProvider> context_provider,
            const SchedulerCallbacks& scheduler_callbacks,
            const base::Closure& error_callback);
  ~GpuOutput() override;

  Scheduler* GetScheduler() override;
  void SubmitFrame(const scoped_refptr<RenderFrame>& frame) override;

 private:
  struct FrameData {
    enum class State {
      Pending,  // initial state waiting for draw to start
      Drawing,  // draw has started
      Finished  // draw has finished
    };

    FrameData(const scoped_refptr<RenderFrame>& frame, int64_t submit_time);
    ~FrameData();

    void ResetDrawState();

    const scoped_refptr<RenderFrame> frame;
    const int64_t submit_time;
    State state = State::Pending;
    int64_t draw_started_time = 0;  // time when drawing began
    int64_t draw_issued_time = 0;   // time when awaiting for finish began

   private:
    DISALLOW_COPY_AND_ASSIGN(FrameData);
  };

  // |GpuRasterizer::Callbacks|:
  void OnRasterizerReady(int64_t vsync_timebase,
                         int64_t vsync_interval) override;
  void OnRasterizerSuspended() override;
  void OnRasterizerFinishedDraw(bool presented) override;
  void OnRasterizerError() override;

  void ScheduleDrawLocked();
  void OnDraw();

  void InitializeRasterizer(
      mojo::InterfaceHandle<mojo::ContextProvider> context_provider);
  void DestroyRasterizer();
  void PostErrorCallback();

  scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner_;
  scoped_refptr<VsyncScheduler> vsync_scheduler_;
  base::Closure error_callback_;

  // Maximum number of frames to hold in the drawing pipeline.
  // Any more than this and we start dropping them.
  uint32_t pipeline_depth_;

  // The rasterizer itself runs on its own thread.
  std::unique_ptr<base::Thread> rasterizer_thread_;
  scoped_refptr<base::SingleThreadTaskRunner> rasterizer_task_runner_;
  base::WaitableEvent rasterizer_initialized_;
  std::unique_ptr<GpuRasterizer> rasterizer_;

  // Holds state shared between the compositor and rasterizer threads.
  struct {
    std::mutex mutex;  // guards all shared state

    // Queue of frames.
    //
    // The head of this queue consists of up to |pipeline_depth| frames
    // which are drawn and awaiting finish.  These frames are popped off
    // the queue when finished unless the queue would become empty (such
    // that we always retain the current frame as the tail).
    //
    // The tail of this queue is a single frame which is either drawn or
    // finished and represents the current (most recently submitted)
    // content.
    //
    // The queue is only ever empty until the first frame is submitted.
    // Subsequently, it always contains at least one frame.
    std::queue<std::unique_ptr<FrameData>> frames;

    // Set to true when the rasterizer is ready to draw.
    bool rasterizer_ready = false;

    // Set to true when a request to draw has been scheduled.
    bool draw_scheduled = false;
  } shared_state_;

  DISALLOW_COPY_AND_ASSIGN(GpuOutput);
};

}  // namespace compositor

#endif  // SERVICES_GFX_COMPOSITOR_BACKEND_GPU_OUTPUT_H_
