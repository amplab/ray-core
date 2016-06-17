// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/ui/noodles/noodles_view.h"

#include <math.h>

#include <cstdlib>
#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "examples/ui/noodles/frame.h"
#include "examples/ui/noodles/rasterizer.h"
#include "mojo/public/cpp/application/connect.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkPicture.h"
#include "third_party/skia/include/core/SkPictureRecorder.h"

namespace examples {

namespace {
constexpr double kSecondsBetweenChanges = 10.0;

template <typename T>
void Drop(scoped_ptr<T> ptr) {}

scoped_ptr<base::MessagePump> CreateMessagePumpMojo() {
  return base::MessageLoop::CreateMessagePumpForType(
      base::MessageLoop::TYPE_DEFAULT);
}

void Lissajous(SkPath* path, double ax, double ay, int wx, int wy, double p) {
  uint32_t segments = ceil(fabs(ax) + fabs(ay)) / 2u + 1u;
  for (uint32_t i = 0; i < segments; ++i) {
    double t = M_PI * 2.0 * i / segments;
    double x = ax * sin(t * wx);
    double y = ay * sin(t * wy + p);
    if (i == 0u)
      path->moveTo(x, y);
    else
      path->lineTo(x, y);
  }
  path->close();
}
}  // namespace

NoodlesView::NoodlesView(
    mojo::InterfaceHandle<mojo::ApplicationConnector> app_connector,
    mojo::InterfaceRequest<mojo::ui::ViewOwner> view_owner_request)
    : BaseView(app_connector.Pass(), view_owner_request.Pass(), "Noodles"),
      frame_queue_(std::make_shared<FrameQueue>()),
      rasterizer_delegate_(
          make_scoped_ptr(new RasterizerDelegate(frame_queue_))) {
  base::Thread::Options options;
  options.message_pump_factory = base::Bind(&CreateMessagePumpMojo);

  rasterizer_thread_.reset(new base::Thread("noodles_rasterizer"));
  rasterizer_thread_->StartWithOptions(options);
  rasterizer_task_runner_ = rasterizer_thread_->message_loop()->task_runner();

  rasterizer_task_runner_->PostTask(
      FROM_HERE, base::Bind(&RasterizerDelegate::CreateRasterizer,
                            base::Unretained(rasterizer_delegate_.get()),
                            base::Passed(mojo::DuplicateApplicationConnector(
                                BaseView::app_connector())),
                            base::Passed(TakeScene().PassInterfaceHandle())));
}

NoodlesView::~NoodlesView() {
  // Ensure destruction happens on the correct thread.
  rasterizer_task_runner_->PostTask(
      FROM_HERE, base::Bind(&Drop<RasterizerDelegate>,
                            base::Passed(&rasterizer_delegate_)));
}

void NoodlesView::OnDraw() {
  DCHECK(properties());

  const mojo::Size& size = *properties()->view_layout->size;

  // Update the animation.
  alpha_ += frame_tracker().frame_time_delta() * 0.000001f;

  // Create and post a new frame to the renderer.
  std::unique_ptr<Frame> frame(
      new Frame(size, CreatePicture(), CreateSceneMetadata()));
  if (frame_queue_->PutFrame(std::move(frame))) {
    rasterizer_task_runner_->PostTask(
        FROM_HERE, base::Bind(&RasterizerDelegate::PublishNextFrame,
                              base::Unretained(rasterizer_delegate_.get())));
  }

  // Animate!
  Invalidate();
}

sk_sp<SkPicture> NoodlesView::CreatePicture() {
  constexpr int count = 4;
  constexpr int padding = 1;

  if (alpha_ > kSecondsBetweenChanges) {
    alpha_ = 0.0;
    wx_ = rand() % 9 + 1;
    wy_ = rand() % 9 + 1;
  }

  const mojo::Size& size = *properties()->view_layout->size;
  SkPictureRecorder recorder;
  SkCanvas* canvas = recorder.beginRecording(size.width, size.height);

  double cx = size.width * 0.5;
  double cy = size.height * 0.5;
  canvas->translate(cx, cy);

  double phase = alpha_;
  for (int i = 0; i < count; i++, phase += 0.1) {
    SkPaint paint;
    SkScalar hsv[3] = {fmod(phase * 120, 360), 1, 1};
    paint.setColor(SkHSVToColor(hsv));
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setAntiAlias(true);

    SkPath path;
    Lissajous(&path, cx - padding, cy - padding, wx_, wy_, phase);
    canvas->drawPath(path, paint);
  }

  return recorder.finishRecordingAsPicture();
}

NoodlesView::FrameQueue::FrameQueue() {}

NoodlesView::FrameQueue::~FrameQueue() {}

bool NoodlesView::FrameQueue::PutFrame(std::unique_ptr<Frame> frame) {
  std::lock_guard<std::mutex> lock(mutex_);
  bool was_empty = !next_frame_.get();
  next_frame_.swap(frame);
  return was_empty;
}

std::unique_ptr<Frame> NoodlesView::FrameQueue::TakeFrame() {
  std::lock_guard<std::mutex> lock(mutex_);
  return std::move(next_frame_);
}

NoodlesView::RasterizerDelegate::RasterizerDelegate(
    const std::shared_ptr<FrameQueue>& frame_queue)
    : frame_queue_(frame_queue) {
  DCHECK(frame_queue_);
}

NoodlesView::RasterizerDelegate::~RasterizerDelegate() {}

void NoodlesView::RasterizerDelegate::CreateRasterizer(
    mojo::InterfaceHandle<mojo::ApplicationConnector> connector_info,
    mojo::InterfaceHandle<mojo::gfx::composition::Scene> scene_info) {
  rasterizer_.reset(new Rasterizer(
      mojo::ApplicationConnectorPtr::Create(std::move(connector_info)),
      mojo::gfx::composition::ScenePtr::Create(std::move(scene_info))));
}

void NoodlesView::RasterizerDelegate::PublishNextFrame() {
  std::unique_ptr<Frame> frame(frame_queue_->TakeFrame());
  DCHECK(frame);
  rasterizer_->PublishFrame(std::move(frame));
}

}  // namespace examples
