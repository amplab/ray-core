// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_SERVICES_MEDIA_FACTORY_TIMELINE_CONTROLLER_IMPL_H_
#define MOJO_SERVICES_MEDIA_FACTORY_TIMELINE_CONTROLLER_IMPL_H_

#include <memory>
#include <vector>

#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/services/media/common/cpp/local_time.h"
#include "mojo/services/media/common/cpp/timeline.h"
#include "mojo/services/media/common/cpp/timeline_function.h"
#include "mojo/services/media/core/interfaces/timeline_controller.mojom.h"
#include "services/media/common/mojo_publisher.h"
#include "services/media/factory_service/factory_service.h"
#include "services/media/framework/util/callback_joiner.h"

namespace mojo {
namespace media {

// Mojo agent that controls timing in a graph.
class MediaTimelineControllerImpl
    : public MediaFactoryService::Product<MediaTimelineController>,
      public MediaTimelineController,
      public MediaTimelineControlSite,
      public TimelineConsumer {
 public:
  static std::shared_ptr<MediaTimelineControllerImpl> Create(
      InterfaceRequest<MediaTimelineController> request,
      MediaFactoryService* owner);

  ~MediaTimelineControllerImpl() override;

  // MediaTimelineController implementation.
  void AddControlSite(
      InterfaceHandle<MediaTimelineControlSite> control_site) override;

  void GetControlSite(
      InterfaceRequest<MediaTimelineControlSite> control_site) override;

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
  static constexpr int64_t kDefaultLeadTime = Timeline::ns_from_ms(30);

  // Relationship to subordinate control site.
  struct SiteState {
    SiteState(MediaTimelineControllerImpl* parent,
              MediaTimelineControlSitePtr site);

    ~SiteState();

    void HandleStatusUpdates(
        uint64_t version = MediaTimelineControlSite::kInitialStatus,
        MediaTimelineControlSiteStatusPtr status = nullptr);

    MediaTimelineControllerImpl* parent_;
    MediaTimelineControlSitePtr site_;
    TimelineConsumerPtr consumer_;
    bool end_of_stream_ = false;
  };

  class TimelineTransition
      : public std::enable_shared_from_this<TimelineTransition> {
   public:
    TimelineTransition(int64_t reference_time,
                       int64_t subject_time,
                       uint32_t reference_delta,
                       uint32_t subject_delta,
                       const SetTimelineTransformCallback& callback);

    ~TimelineTransition();

    // Calls returns a new callback for a child (site) transition. THIS METHOD
    // WILL ONLY WORK IF THERE IS ALREADY A SHARED POINTER TO THIS OBJECT.
    std::function<void(bool)> NewCallback() {
      callback_joiner_.Spawn();

      std::shared_ptr<TimelineTransition> this_ptr = shared_from_this();
      DCHECK(!this_ptr.unique());

      return [this_ptr](bool completed) {
        DCHECK(this_ptr);
        if (!completed && !this_ptr->cancelled_) {
          LOG(WARNING) << "A site transition was cancelled unexpectedly.";
        }
        this_ptr->callback_joiner_.Complete();
      };
    }

    // Cancels this transition.
    void Cancel() {
      DCHECK(!cancelled_);
      cancelled_ = true;
      DCHECK(callback_.is_null());
      callback_.Run(false);
      callback_.reset();
      completed_callback_.reset();
    }

    // Specifies a callback to be called if and when the transition is complete.
    // The callback will never be called if the transition is cancelled.
    void WhenCompleted(const mojo::Callback<void()>& completed_callback) {
      DCHECK(completed_callback_.is_null());
      if (callback_.is_null() && !cancelled_) {
        completed_callback.Run();
      } else {
        completed_callback_ = completed_callback;
      }
    }

    // Returns the TimelineFunction that will result from this transition.
    const TimelineFunction& new_timeline_function() const {
      return new_timeline_function_;
    }

   private:
    TimelineFunction new_timeline_function_;
    SetTimelineTransformCallback callback_;
    CallbackJoiner callback_joiner_;
    bool cancelled_ = false;
    Callback<void()> completed_callback_;
  };

  MediaTimelineControllerImpl(InterfaceRequest<MediaTimelineController> request,
                              MediaFactoryService* owner);

  // Takes action when a site changes its end-of-stream value.
  void HandleSiteEndOfStreamChange();

  Binding<MediaTimelineControlSite> control_site_binding_;
  Binding<TimelineConsumer> consumer_binding_;
  MojoPublisher<GetStatusCallback> status_publisher_;
  std::vector<std::unique_ptr<SiteState>> site_states_;
  TimelineFunction current_timeline_function_;
  bool end_of_stream_ = false;
  std::weak_ptr<TimelineTransition> pending_transition_;
};

}  // namespace media
}  // namespace mojo

#endif  // MOJO_SERVICES_MEDIA_FACTORY_TIMELINE_CONTROLLER_IMPL_H_
