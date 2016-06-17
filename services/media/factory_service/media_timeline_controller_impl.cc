// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "mojo/services/media/common/cpp/timeline.h"
#include "services/media/factory_service/media_timeline_controller_impl.h"
#include "services/media/framework_mojo/mojo_type_conversions.h"

namespace mojo {
namespace media {

// static
std::shared_ptr<MediaTimelineControllerImpl>
MediaTimelineControllerImpl::Create(
    InterfaceRequest<MediaTimelineController> request,
    MediaFactoryService* owner) {
  return std::shared_ptr<MediaTimelineControllerImpl>(
      new MediaTimelineControllerImpl(request.Pass(), owner));
}

MediaTimelineControllerImpl::MediaTimelineControllerImpl(
    InterfaceRequest<MediaTimelineController> request,
    MediaFactoryService* owner)
    : MediaFactoryService::Product<MediaTimelineController>(this,
                                                            request.Pass(),
                                                            owner),
      control_site_binding_(this),
      consumer_binding_(this) {
  status_publisher_.SetCallbackRunner(
      [this](const GetStatusCallback& callback, uint64_t version) {
        MediaTimelineControlSiteStatusPtr status =
            MediaTimelineControlSiteStatus::New();
        status->timeline_transform =
            mojo::TimelineTransform::From(current_timeline_function_);
        status->end_of_stream = end_of_stream_;
        callback.Run(version, status.Pass());
      });
}

MediaTimelineControllerImpl::~MediaTimelineControllerImpl() {
  status_publisher_.SendUpdates();
}

void MediaTimelineControllerImpl::AddControlSite(
    InterfaceHandle<MediaTimelineControlSite> control_site) {
  site_states_.push_back(std::unique_ptr<SiteState>(new SiteState(
      this, MediaTimelineControlSitePtr::Create(std::move(control_site)))));

  site_states_.back()->HandleStatusUpdates();
}

void MediaTimelineControllerImpl::GetControlSite(
    InterfaceRequest<MediaTimelineControlSite> control_site) {
  if (control_site_binding_.is_bound()) {
    control_site_binding_.Close();
  }

  control_site_binding_.Bind(control_site.Pass());
}

void MediaTimelineControllerImpl::GetStatus(uint64_t version_last_seen,
                                            const GetStatusCallback& callback) {
  status_publisher_.Get(version_last_seen, callback);
}

void MediaTimelineControllerImpl::GetTimelineConsumer(
    InterfaceRequest<TimelineConsumer> timeline_consumer) {
  if (consumer_binding_.is_bound()) {
    consumer_binding_.Close();
  }

  consumer_binding_.Bind(timeline_consumer.Pass());
}

void MediaTimelineControllerImpl::SetTimelineTransform(
    int64_t subject_time,
    uint32_t reference_delta,
    uint32_t subject_delta,
    int64_t effective_reference_time,
    int64_t effective_subject_time,
    const SetTimelineTransformCallback& callback) {
  // At most one effective time may be specified.
  RCHECK(effective_subject_time == kUnspecifiedTime ||
         effective_reference_time == kUnspecifiedTime);
  // effective_subject_time can only be specified if we're progressing.
  RCHECK(effective_subject_time == kUnspecifiedTime ||
         current_timeline_function_.subject_delta() != 0u);
  RCHECK(reference_delta != 0);

  // There can only be once SetTimelineTransform transition pending at any
  // moment, so a new SetTimelineTransform call that arrives before a previous
  // one completes cancels the previous one. This causes some problems for us,
  // because some sites may complete the previous transition while other may
  // not.
  //
  // We start by noticing that there's an incomplete previous transition, and
  // we 'cancel' it, meaning we call its callback with a false complete
  // parameter.
  //
  // If we're cancelling a previous transition, we need to take steps to make
  // sure the sites will end up in the right state regardless of whether they
  // completed the previous transition. We do two things:
  //
  // 1) If subject_time isn't specified, we infer it here and supply the
  //    inferred value to the sites, so there's no disagreement about its
  //    value.
  // 2) If the effective_subject_time is specified rather than the
  //    effective_reference_time, we infer effective_reference_time and send
  //    it to the sites instead of effective_subject_time, so there's no
  //    disagreement about effective time and so that no sites reject the
  //    transition due to having a zero subject_delta.

  std::shared_ptr<TimelineTransition> pending_transition =
      pending_transition_.lock();
  if (pending_transition) {
    // A transition is pending - cancel it.
    pending_transition->Cancel();
  }

  // These will be recorded as part of the new TimelineFunction.
  int64_t new_reference_time;
  int64_t new_subject_time;

  if (effective_subject_time != kUnspecifiedTime) {
    // Infer new_reference_time from effective_subject_time.
    new_reference_time =
        current_timeline_function_.ApplyInverse(effective_subject_time);

    // Figure out what the subject_time will be after this transition.
    if (subject_time == kUnspecifiedTime) {
      new_subject_time = effective_subject_time;
    } else {
      new_subject_time = subject_time;
    }
  } else {
    if (effective_reference_time == kUnspecifiedTime) {
      // Neither effective time was specified. Effective time is now.
      effective_reference_time = Timeline::local_now() + kDefaultLeadTime;
    }

    // new_reference_time is just effective_reference_time.
    new_reference_time = effective_reference_time;

    // Figure out what the subject_time will be after this transition.
    if (subject_time == kUnspecifiedTime) {
      new_subject_time = current_timeline_function_(effective_reference_time);
    } else {
      new_subject_time = subject_time;
    }
  }

  if (pending_transition) {
    // This transition cancels a previous one. Use effective_reference_time
    // rather than effective_subject_time, because we can't be sure what
    // effective_subject_time will mean to the sites.
    effective_reference_time = new_reference_time;
    effective_subject_time = kUnspecifiedTime;

    // We don't want the sites to have to infer the subject_time, because we
    // can't be sure what subject_time a site will infer.
    subject_time = new_subject_time;
  }

  // Recording the new pending transition.
  std::shared_ptr<TimelineTransition> transition =
      std::shared_ptr<TimelineTransition>(
          new TimelineTransition(new_reference_time, new_subject_time,
                                 reference_delta, subject_delta, callback));

  pending_transition_ = transition;

  // Initiate the transition for each site.
  for (const std::unique_ptr<SiteState>& site_state : site_states_) {
    site_state->consumer_->SetTimelineTransform(
        subject_time, reference_delta, subject_delta, effective_reference_time,
        effective_subject_time, transition->NewCallback());
  }

  // If and when this transition is complete, adopt the new TimelineFunction
  // and tell any status subscribers.
  transition->WhenCompleted([this, transition]() {
    current_timeline_function_ = transition->new_timeline_function();
    status_publisher_.SendUpdates();
  });
}

void MediaTimelineControllerImpl::HandleSiteEndOfStreamChange() {
  bool end_of_stream = true;
  for (const std::unique_ptr<SiteState>& site_state : site_states_) {
    if (!site_state->end_of_stream_) {
      end_of_stream = false;
      break;
    }
  }

  if (end_of_stream_ != end_of_stream) {
    end_of_stream_ = end_of_stream;
    status_publisher_.SendUpdates();
  }
}

MediaTimelineControllerImpl::SiteState::SiteState(
    MediaTimelineControllerImpl* parent,
    MediaTimelineControlSitePtr site)
    : parent_(parent), site_(site.Pass()) {
  site_->GetTimelineConsumer(GetProxy(&consumer_));
}

MediaTimelineControllerImpl::SiteState::~SiteState() {}

void MediaTimelineControllerImpl::SiteState::HandleStatusUpdates(
    uint64_t version,
    MediaTimelineControlSiteStatusPtr status) {
  if (status) {
    // Respond to any end-of-stream changes.
    if (end_of_stream_ != status->end_of_stream) {
      end_of_stream_ = status->end_of_stream;
      parent_->HandleSiteEndOfStreamChange();
    }
  }

  site_->GetStatus(version, [this](uint64_t version,
                                   MediaTimelineControlSiteStatusPtr status) {
    HandleStatusUpdates(version, status.Pass());
  });
}

MediaTimelineControllerImpl::TimelineTransition::TimelineTransition(
    int64_t reference_time,
    int64_t subject_time,
    uint32_t reference_delta,
    uint32_t subject_delta,
    const SetTimelineTransformCallback& callback)
    : new_timeline_function_(reference_time,
                             subject_time,
                             reference_delta,
                             subject_delta),
      callback_(callback) {
  DCHECK(!callback_.is_null());
  callback_joiner_.WhenJoined([this]() {
    if (cancelled_) {
      DCHECK(callback_.is_null());
      return;
    }

    DCHECK(!callback_.is_null());
    callback_.Run(true);
    callback_.reset();
    if (!completed_callback_.is_null()) {
      completed_callback_.Run();
      completed_callback_.reset();
    }
  });
}

MediaTimelineControllerImpl::TimelineTransition::~TimelineTransition() {}

}  // namespace media
}  // namespace mojo
