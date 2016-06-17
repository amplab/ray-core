// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/gfx/compositor/backend/gpu_output.h"

#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/trace_event/trace_event.h"
#include "services/gfx/compositor/render/render_frame.h"

namespace compositor {
namespace {
constexpr const char* kPipelineDepthSwitch = "pipeline-depth";
constexpr uint32_t kDefaultPipelineDepth = 1u;
constexpr uint32_t kMinPipelineDepth = 1u;
constexpr uint32_t kMaxPipelineDepth = 10u;  // for experimentation

scoped_ptr<base::MessagePump> CreateMessagePumpMojo() {
  return base::MessageLoop::CreateMessagePumpForType(
      base::MessageLoop::TYPE_DEFAULT);
}
}  // namespace

GpuOutput::GpuOutput(
    mojo::InterfaceHandle<mojo::ContextProvider> context_provider,
    const SchedulerCallbacks& scheduler_callbacks,
    const base::Closure& error_callback)
    : compositor_task_runner_(base::MessageLoop::current()->task_runner()),
      vsync_scheduler_(
          new VsyncScheduler(compositor_task_runner_, scheduler_callbacks)),
      error_callback_(error_callback),
      rasterizer_thread_(new base::Thread("gpu_rasterizer")),
      rasterizer_initialized_(true, false) {
  DCHECK(context_provider);

  pipeline_depth_ = kDefaultPipelineDepth;
  auto command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(kPipelineDepthSwitch)) {
    std::string str(command_line->GetSwitchValueASCII(kPipelineDepthSwitch));
    unsigned value;
    if (base::StringToUint(str, &value) && value >= kMinPipelineDepth &&
        value <= kMaxPipelineDepth) {
      pipeline_depth_ = value;
    } else {
      LOG(ERROR) << "Invalid value for --" << kPipelineDepthSwitch << ": \""
                 << str << "\"";
      PostErrorCallback();
    }
  }
  DVLOG(2) << "Using pipeline depth " << pipeline_depth_;

  base::Thread::Options options;
  options.message_pump_factory = base::Bind(&CreateMessagePumpMojo);
  rasterizer_thread_->StartWithOptions(options);
  rasterizer_task_runner_ = rasterizer_thread_->message_loop()->task_runner();
  rasterizer_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&GpuOutput::InitializeRasterizer, base::Unretained(this),
                 base::Passed(std::move(context_provider))));
  rasterizer_initialized_.Wait();
  DCHECK(rasterizer_);
}

GpuOutput::~GpuOutput() {
  // Ensure rasterizer destruction happens on the rasterizer thread.
  rasterizer_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&GpuOutput::DestroyRasterizer, base::Unretained(this)));
  rasterizer_thread_->Stop();
  DCHECK(!rasterizer_);
}

Scheduler* GpuOutput::GetScheduler() {
  return vsync_scheduler_.get();
}

void GpuOutput::SubmitFrame(const scoped_refptr<RenderFrame>& frame) {
  DCHECK(frame);
  TRACE_EVENT0("gfx", "GpuOutput::SubmitFrame");

  const int64_t submit_time = MojoGetTimeTicksNow();

  // Note: we may swap an old frame into |frame_data| to keep alive until
  // we exit the lock.
  std::unique_ptr<FrameData> frame_data(new FrameData(frame, submit_time));
  {
    std::lock_guard<std::mutex> lock(shared_state_.mutex);

    // Enqueue the frame, ensuring that the queue only contains at most
    // one pending or scheduled frame.  If the last frame hasn't been drawn by
    // now then the rasterizer must be falling behind.
    if (shared_state_.frames.empty() ||
        shared_state_.frames.back()->state == FrameData::State::Drawing) {
      // The queue is busy drawing.  Enqueue the new frame at the end.
      shared_state_.frames.emplace(std::move(frame_data));
    } else if (shared_state_.frames.back()->state ==
               FrameData::State::Finished) {
      // The queue contains a finished frame which we had retained to prevent
      // the queue from becoming empty and losing track of the current frame.
      // Replace it with the new frame.
      DCHECK(shared_state_.frames.size() == 1u);
      shared_state_.frames.back().swap(frame_data);
    } else {
      // The queue already contains a pending frame which means the rasterizer
      // has gotten so far behind it wasn't even able to issue the previous
      // undrawn frame.  Replace it with the new frame, thereby ensuring
      // the queue never contains more than one pending frame at a time.
      DCHECK(shared_state_.frames.back()->state == FrameData::State::Pending);
      shared_state_.frames.back().swap(frame_data);
      TRACE_EVENT_FLOW_END1("gfx", "Frame Queued", frame_data.get(), "drawn",
                            false);
      DVLOG(2) << "Rasterizer stalled, dropped a frame to catch up.";
    }

    TRACE_EVENT_FLOW_BEGIN0("gfx", "Frame Queued",
                            shared_state_.frames.back().get());

    if (!shared_state_.rasterizer_ready)
      return;

    // TODO(jeffbrown): If the draw queue is overfull, we should pause
    // scheduling until the queue drains.
    if (shared_state_.frames.size() <= pipeline_depth_)
      ScheduleDrawLocked();
  }
}

void GpuOutput::OnRasterizerReady(int64_t vsync_timebase,
                                  int64_t vsync_interval) {
  DCHECK(rasterizer_task_runner_->RunsTasksOnCurrentThread());

  // TODO(jeffbrown): This shouldn't be hardcoded.
  // Need to do some real tuning and possibly determine values adaptively.
  // We should probably split the Start() method in two to separate the
  // process of setting parameters from starting / stopping scheduling.
  const int64_t update_phase = -vsync_interval;
  const int64_t snapshot_phase = -vsync_interval / 6;
  // TODO(jeffbrown): Determine the presentation phase based on queue depth.
  const int64_t presentation_phase = vsync_interval * pipeline_depth_;
  if (!vsync_scheduler_->Start(vsync_timebase, vsync_interval, update_phase,
                               snapshot_phase, presentation_phase)) {
    LOG(ERROR) << "Received invalid vsync parameters: timebase="
               << vsync_timebase << ", interval=" << vsync_interval;
    PostErrorCallback();
    return;
  }

  {
    std::lock_guard<std::mutex> lock(shared_state_.mutex);

    if (shared_state_.rasterizer_ready)
      return;

    shared_state_.rasterizer_ready = true;

    if (shared_state_.frames.empty())
      return;

    shared_state_.frames.back()->ResetDrawState();
    TRACE_EVENT_FLOW_BEGIN0("gfx", "Frame Queued",
                            shared_state_.frames.back().get());
    ScheduleDrawLocked();
  }
}

void GpuOutput::OnRasterizerSuspended() {
  DCHECK(rasterizer_task_runner_->RunsTasksOnCurrentThread());

  vsync_scheduler_->Stop();

  {
    std::lock_guard<std::mutex> lock(shared_state_.mutex);

    if (!shared_state_.rasterizer_ready)
      return;

    shared_state_.rasterizer_ready = false;
  }
}

void GpuOutput::OnRasterizerError() {
  DCHECK(rasterizer_task_runner_->RunsTasksOnCurrentThread());

  PostErrorCallback();
}

void GpuOutput::ScheduleDrawLocked() {
  DCHECK(!shared_state_.frames.empty());
  DCHECK(shared_state_.frames.back()->state == FrameData::State::Pending);
  DCHECK(shared_state_.frames.size() <= pipeline_depth_);

  if (shared_state_.draw_scheduled)
    return;

  shared_state_.draw_scheduled = true;
  rasterizer_task_runner_->PostTask(
      FROM_HERE, base::Bind(&GpuOutput::OnDraw, base::Unretained(this)));
}

void GpuOutput::OnDraw() {
  DCHECK(rasterizer_task_runner_->RunsTasksOnCurrentThread());
  TRACE_EVENT0("gfx", "GpuOutput::OnDraw");

  FrameData* frame_data;  // used outside lock
  {
    std::lock_guard<std::mutex> lock(shared_state_.mutex);

    DCHECK(shared_state_.draw_scheduled);
    DCHECK(!shared_state_.frames.empty());
    DCHECK(shared_state_.frames.back()->state == FrameData::State::Pending);

    shared_state_.draw_scheduled = false;
    if (!shared_state_.rasterizer_ready)
      return;

    frame_data = shared_state_.frames.back().get();
    frame_data->state = FrameData::State::Drawing;
    frame_data->draw_started_time = MojoGetTimeTicksNow();
    TRACE_EVENT_FLOW_END1("gfx", "Frame Queued", frame_data, "drawn", true);
  }

  // It is safe to access |frame_data| outside of the lock here because
  // it will not be dequeued until |OnRasterizerFinishedDraw| gets posted
  // to this thread's message loop.  Moreover |SubmitFrame| will not discard
  // or replace the frame because its state is |Drawing|.
  TRACE_EVENT_ASYNC_BEGIN0("gfx", "Rasterize", frame_data);
  rasterizer_->DrawFrame(frame_data->frame);
  frame_data->draw_issued_time = MojoGetTimeTicksNow();
}

void GpuOutput::OnRasterizerFinishedDraw(bool presented) {
  DCHECK(rasterizer_task_runner_->RunsTasksOnCurrentThread());
  TRACE_EVENT0("gfx", "GpuOutput::OnRasterizerFinishedDraw");

  const int64_t finish_time = MojoGetTimeTicksNow();

  // Note: we may swap an old frame into |old_frame_data| to keep alive until
  // we exit the lock.
  std::unique_ptr<FrameData> old_frame_data;
  {
    std::lock_guard<std::mutex> lock(shared_state_.mutex);

    DCHECK(shared_state_.rasterizer_ready);
    DCHECK(!shared_state_.frames.empty());

    FrameData* frame_data = shared_state_.frames.front().get();
    DCHECK(frame_data);
    DCHECK(frame_data->state == FrameData::State::Drawing);
    TRACE_EVENT_ASYNC_END1("gfx", "Rasterize", frame_data, "presented",
                           presented);

    frame_data->state = FrameData::State::Finished;

    // TODO(jeffbrown): Adjust scheduler behavior based on observed timing.
    // Note: These measurements don't account for systematic downstream delay
    // in the display pipeline (how long it takes pixels to actually light up).
    if (presented) {
      const RenderFrame::Metadata& frame_metadata =
          frame_data->frame->metadata();
      const mojo::gfx::composition::FrameInfo& frame_info =
          frame_metadata.frame_info();
      const int64_t frame_time = frame_info.frame_time;
      const int64_t presentation_time = frame_info.presentation_time;
      const int64_t composition_time = frame_metadata.composition_time();
      const int64_t draw_started_time = frame_data->draw_started_time;
      const int64_t draw_issued_time = frame_data->draw_issued_time;
      const int64_t submit_time = frame_data->submit_time;
      const size_t draw_queue_depth = shared_state_.frames.size();

      DVLOG(2) << "Presented frame: composition latency "
               << (composition_time - frame_time) << " us, submission latency "
               << (submit_time - composition_time) << " us, queue latency "
               << (draw_started_time - submit_time) << " us, draw latency "
               << (draw_issued_time - draw_started_time) << " us, GPU latency "
               << (finish_time - draw_issued_time) << " us, total latency "
               << (finish_time - frame_time) << " us, presentation time error "
               << (finish_time - presentation_time) << " us"
               << ", draw queue depth " << draw_queue_depth;
    } else {
      DVLOG(2) << "Rasterizer dropped frame.";
    }

    if (shared_state_.frames.size() > 1u) {
      shared_state_.frames.front().swap(old_frame_data);
      shared_state_.frames.pop();
      if (shared_state_.frames.back()->state == FrameData::State::Pending)
        ScheduleDrawLocked();
    }
  }
}

void GpuOutput::InitializeRasterizer(
    mojo::InterfaceHandle<mojo::ContextProvider> context_provider) {
  DCHECK(rasterizer_task_runner_->RunsTasksOnCurrentThread());
  DCHECK(!rasterizer_);
  TRACE_EVENT0("gfx", "GpuOutput::InitializeRasterizer");

  rasterizer_.reset(new GpuRasterizer(
      mojo::ContextProviderPtr::Create(std::move(context_provider)), this));
  rasterizer_initialized_.Signal();
}

void GpuOutput::DestroyRasterizer() {
  DCHECK(rasterizer_task_runner_->RunsTasksOnCurrentThread());
  DCHECK(rasterizer_);
  TRACE_EVENT0("gfx", "GpuOutput::DestroyRasterizer");

  rasterizer_.reset();
  rasterizer_initialized_.Reset();
}

void GpuOutput::PostErrorCallback() {
  compositor_task_runner_->PostTask(FROM_HERE, error_callback_);
}

GpuOutput::FrameData::FrameData(const scoped_refptr<RenderFrame>& frame,
                                int64_t submit_time)
    : frame(frame), submit_time(submit_time) {}

GpuOutput::FrameData::~FrameData() {}

void GpuOutput::FrameData::ResetDrawState() {
  state = State::Pending;
  draw_started_time = 0;
  draw_issued_time = 0;
}

}  // namespace compositor
