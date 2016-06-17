// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_SERVICES_MEDIA_COMMON_TIMELINE_CONTROL_SITE_IMPL_H_
#define MOJO_SERVICES_MEDIA_COMMON_TIMELINE_CONTROL_SITE_IMPL_H_

#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/services/media/common/cpp/timeline_function.h"
#include "mojo/services/media/core/interfaces/timeline_controller.mojom.h"
#include "services/media/common/mojo_publisher.h"

namespace mojo {
namespace media {

// MediaTimelineControlSite implementation.
class TimelineControlSite : public MediaTimelineControlSite,
                            public TimelineConsumer {
 public:
  TimelineControlSite();

  ~TimelineControlSite() override;

  // Binds to the control site. If a binding exists already, it is closed.
  void Bind(InterfaceRequest<MediaTimelineControlSite> request);

  // Determines whether the control site is currently bound.
  bool is_bound() { return control_site_binding_.is_bound(); }

  // Unbinds from clients and resets to initial state.
  void Reset();

  // Get the TimelineFunction for the reference_time (which should be 'now',
  // approximately).
  void SnapshotCurrentFunction(int64_t reference_time,
                               TimelineFunction* out,
                               uint32_t* generation = nullptr);

  // Sets the current end_of_stream status published by the control site.
  void SetEndOfStreamPts(int64_t end_of_stream_pts);

  // MediaTimelineControlSite implementation.
  void GetStatus(uint64_t version_last_seen,
                 const GetStatusCallback& callback) override;

  void GetTimelineConsumer(
      InterfaceRequest<TimelineConsumer> timeline_consumer) override;

  // TimelineConsumer implementation.
  void SetTimelineTransform(
      int64_t subject_time,
      uint32_t reference_delta,
      uint32_t subject_delta,
      int64_t effective_reference_time,
      int64_t effective_subject_time,
      const SetTimelineTransformCallback& callback) override;

 private:
  // Applies pending_timeline_function_ if it's time to do so based on the
  // given reference time.
  void ApplyPendingChangesUnsafe(int64_t reference_time);

  // Clears the pending timeline function and calls its associated callback
  // with the indicated completed status.
  void ClearPendingTimelineFunctionUnsafe(bool completed);

  // Determines if an unrealized timeline function is currently pending.
  bool TimelineFunctionPendingUnsafe() {
    return pending_timeline_function_.reference_time() != kUnspecifiedTime;
  }

  // Determines whether end-of-stream has been reached.
  bool ReachedEndOfStreamUnsafe();

  // Unbinds from clients and resets to initial state.
  void ResetUnsafe();

  static void RunCallback(SetTimelineTransformCallback callback,
                          bool completed);

  Binding<MediaTimelineControlSite> control_site_binding_;
  Binding<TimelineConsumer> consumer_binding_;
  MojoPublisher<GetStatusCallback> status_publisher_;

  base::Lock lock_;
  // BEGIN fields synchronized using lock_.
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  TimelineFunction current_timeline_function_;
  TimelineFunction pending_timeline_function_;
  SetTimelineTransformCallback set_timeline_transform_callback_;
  uint32_t generation_ = 1;
  int64_t end_of_stream_pts_ = kUnspecifiedTime;
  bool end_of_stream_published_ = false;
  // END fields synchronized using lock_.
};

}  // namespace media
}  // namespace mojo

#endif  // MOJO_SERVICES_MEDIA_COMMON_TIMELINE_CONTROL_SITE_IMPL_H_
