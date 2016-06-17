// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/native_viewport/platform_viewport_android.h"

#include <android/input.h>
#include <android/native_window_jni.h>

#include "base/android/jni_android.h"
#include "base/bind.h"
#include "jni/PlatformViewportAndroid_jni.h"
#include "mojo/converters/geometry/geometry_type_converters.h"
#include "mojo/converters/input_events/input_events_type_converters.h"
#include "mojo/public/cpp/application/connect.h"
#include "services/native_viewport/native_viewport_support.mojom.h"
#include "ui/events/event.h"
#include "ui/events/keycodes/keyboard_code_conversion_android.h"
#include "ui/gfx/point.h"

namespace native_viewport {
namespace {

mojo::EventType MotionEventActionToEventType(jint action) {
  switch (action) {
    case AMOTION_EVENT_ACTION_DOWN:
    case AMOTION_EVENT_ACTION_POINTER_DOWN:
      return mojo::EventType::POINTER_DOWN;
    case AMOTION_EVENT_ACTION_UP:
    case AMOTION_EVENT_ACTION_POINTER_UP:
      return mojo::EventType::POINTER_UP;
    case AMOTION_EVENT_ACTION_MOVE:
      return mojo::EventType::POINTER_MOVE;
    case AMOTION_EVENT_ACTION_CANCEL:
      return mojo::EventType::POINTER_CANCEL;
    case AMOTION_EVENT_ACTION_OUTSIDE:
    case AMOTION_EVENT_ACTION_HOVER_MOVE:
    case AMOTION_EVENT_ACTION_SCROLL:
    case AMOTION_EVENT_ACTION_HOVER_ENTER:
    case AMOTION_EVENT_ACTION_HOVER_EXIT:
    default:
      NOTIMPLEMENTED() << "Unimplemented motion action: " << action;
  }
  return mojo::EventType::UNKNOWN;
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// PlatformViewportAndroid, public:

// static
bool PlatformViewportAndroid::Register(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

PlatformViewportAndroid::PlatformViewportAndroid(mojo::Shell* shell,
                                                 Delegate* delegate)
    : shell_(shell),
      delegate_(delegate),
      window_(NULL),
      id_generator_(0),
      weak_factory_(this) {}

PlatformViewportAndroid::~PlatformViewportAndroid() {
  if (window_)
    ReleaseWindow();
  JNIEnv* env = base::android::AttachCurrentThread();
  if (!java_platform_viewport_android_.is_empty()) {
    Java_PlatformViewportAndroid_detach(
        env, java_platform_viewport_android_.get(env).obj());
  } else {
    Java_PlatformViewportAndroid_withdrawRequest(
        env, reinterpret_cast<intptr_t>(this));
  }
}

void PlatformViewportAndroid::Destroy(JNIEnv* env, jobject obj) {
  delegate_->OnDestroyed();
}

void PlatformViewportAndroid::SurfaceAttached(JNIEnv* env,
                                              jobject obj,
                                              jobject platform_viewport) {
  java_platform_viewport_android_ =
      JavaObjectWeakGlobalRef(env, platform_viewport);
}

void PlatformViewportAndroid::SurfaceCreated(JNIEnv* env,
                                             jobject obj,
                                             jobject jsurface) {
  base::android::ScopedJavaLocalRef<jobject> protector(env, jsurface);
  // Note: This ensures that any local references used by
  // ANativeWindow_fromSurface are released immediately. This is needed as a
  // workaround for https://code.google.com/p/android/issues/detail?id=68174
  {
    base::android::ScopedJavaLocalFrame scoped_local_reference_frame(env);
    window_ = ANativeWindow_fromSurface(env, jsurface);
  }
  delegate_->OnAcceleratedWidgetAvailable(window_);
}

void PlatformViewportAndroid::SurfaceDestroyed(JNIEnv* env, jobject obj) {
  DCHECK(window_);
  delegate_->OnAcceleratedWidgetDestroyed();
  ReleaseWindow();
}

void PlatformViewportAndroid::SurfaceSetSize(JNIEnv* env,
                                             jobject obj,
                                             jint width,
                                             jint height,
                                             jfloat density) {
  metrics_ = mojo::ViewportMetrics::New();
  metrics_->size = mojo::Size::New();
  metrics_->size->width = static_cast<int>(width);
  metrics_->size->height = static_cast<int>(height);
  metrics_->device_pixel_ratio = density;
  delegate_->OnMetricsChanged(metrics_.Clone());
}

bool PlatformViewportAndroid::TouchEvent(JNIEnv* env,
                                         jobject obj,
                                         jlong time_ms,
                                         jint masked_action,
                                         jint pointer_id,
                                         jfloat x,
                                         jfloat y,
                                         jfloat pressure,
                                         jfloat touch_major,
                                         jfloat touch_minor,
                                         jfloat orientation,
                                         jfloat h_wheel,
                                         jfloat v_wheel) {
  mojo::EventPtr event(mojo::Event::New());
  event->time_stamp =
      (base::TimeTicks() + base::TimeDelta::FromMilliseconds(time_ms))
          .ToInternalValue();
  event->action = MotionEventActionToEventType(masked_action);
  if (event->action == mojo::EventType::UNKNOWN)
    return false;

  event->pointer_data = mojo::PointerData::New();
  event->pointer_data->pointer_id = pointer_id;
  event->pointer_data->x = x;
  event->pointer_data->y = y;
  event->pointer_data->screen_x = x;
  event->pointer_data->screen_y = y;
  event->pointer_data->pressure = pressure;
  event->pointer_data->radius_major = touch_major;
  event->pointer_data->radius_minor = touch_minor;
  event->pointer_data->orientation = orientation;
  event->pointer_data->horizontal_wheel = h_wheel;
  event->pointer_data->vertical_wheel = v_wheel;
  delegate_->OnEvent(event.Pass());

  return true;
}

////////////////////////////////////////////////////////////////////////////////
// PlatformViewportAndroid, PlatformViewport implementation:

void PlatformViewportAndroid::Init(const gfx::Rect& bounds) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PlatformViewportAndroid_createRequest(env,
                                             reinterpret_cast<intptr_t>(this));

  mojo::ConnectToService(shell_, "mojo:native_viewport_support",
                         GetProxy(&support_service_));
  support_service_->CreateNewNativeWindow(
      base::Bind(&PlatformViewportAndroid::Close, weak_factory_.GetWeakPtr()));
}

void PlatformViewportAndroid::Show() {
  // Nothing to do. View is created visible.
}

void PlatformViewportAndroid::Hide() {
  // Nothing to do. View is always visible.
}

void PlatformViewportAndroid::Close() {
  // TODO(beng): close activity containing MojoView?

  // TODO(beng): perform this in response to view destruction.
  delegate_->OnDestroyed();
}

gfx::Size PlatformViewportAndroid::GetSize() {
  return metrics_->size.To<gfx::Size>();
}

void PlatformViewportAndroid::SetBounds(const gfx::Rect& bounds) {
  NOTIMPLEMENTED();
}

////////////////////////////////////////////////////////////////////////////////
// PlatformViewportAndroid, private:

void PlatformViewportAndroid::ReleaseWindow() {
  ANativeWindow_release(window_);
  window_ = NULL;
}

////////////////////////////////////////////////////////////////////////////////
// PlatformViewport, public:

// static
scoped_ptr<PlatformViewport> PlatformViewport::Create(mojo::Shell* shell,
                                                      Delegate* delegate) {
  return scoped_ptr<PlatformViewport>(
             new PlatformViewportAndroid(shell, delegate))
      .Pass();
}

}  // namespace native_viewport
