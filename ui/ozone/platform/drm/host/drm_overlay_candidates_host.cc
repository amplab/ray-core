// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/host/drm_overlay_candidates_host.h"

#include <algorithm>

#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/ozone/common/gpu/ozone_gpu_message_params.h"
#include "ui/ozone/platform/drm/host/drm_gpu_platform_support_host.h"

namespace ui {

namespace {
const size_t kMaxCacheSize = 100;
}  // namespace

bool DrmOverlayCandidatesHost::OverlayCompare::operator()(
    const OverlayCheck_Params& l,
    const OverlayCheck_Params& r) {
  if (l.plane_z_order < r.plane_z_order)
    return true;
  if (l.plane_z_order > r.plane_z_order)
    return false;
  if (l.display_rect < r.display_rect)
    return true;
  if (l.display_rect != r.display_rect)
    return false;
  if (l.format < r.format)
    return true;
  if (l.format > r.format)
    return false;
  if (l.transform < r.transform)
    return true;
  if (l.transform > r.transform)
    return false;
  if (l.buffer_size.width() < r.buffer_size.width())
    return true;
  if (l.buffer_size.width() > r.buffer_size.width())
    return false;
  return l.buffer_size.height() < r.buffer_size.height();
}

DrmOverlayCandidatesHost::DrmOverlayCandidatesHost(
    gfx::AcceleratedWidget widget,
    DrmGpuPlatformSupportHost* platform_support)
    : widget_(widget),
      platform_support_(platform_support),
      cache_(kMaxCacheSize) {
}

DrmOverlayCandidatesHost::~DrmOverlayCandidatesHost() {
}

void DrmOverlayCandidatesHost::CheckOverlaySupport(
    OverlaySurfaceCandidateList* candidates) {
  if (candidates->size() == 2)
    CheckSingleOverlay(candidates);
}

void DrmOverlayCandidatesHost::CheckSingleOverlay(
    OverlaySurfaceCandidateList* candidates) {
  OverlayCandidatesOzone::OverlaySurfaceCandidate* first = &(*candidates)[0];
  OverlayCandidatesOzone::OverlaySurfaceCandidate* second = &(*candidates)[1];
  OverlayCandidatesOzone::OverlaySurfaceCandidate* overlay;
  if (first->plane_z_order == 0) {
    overlay = second;
  } else if (second->plane_z_order == 0) {
    overlay = first;
  } else {
    return;
  }
  // 0.01 constant chosen to match DCHECKs in gfx::ToNearestRect and avoid
  // that code asserting on quads that we accept.
  if (!gfx::IsNearestRectWithinDistance(overlay->display_rect, 0.01f))
    return;
  if (overlay->transform == gfx::OVERLAY_TRANSFORM_INVALID)
    return;

  OverlayCheck_Params lookup(*overlay);
  auto iter = cache_.Get(lookup);
  if (iter == cache_.end()) {
    cache_.Put(lookup, false);
    SendRequest(*candidates, lookup);
  } else {
    overlay->overlay_handled = iter->second;
  }
}

void DrmOverlayCandidatesHost::SendRequest(
    const OverlaySurfaceCandidateList& candidates,
    const OverlayCheck_Params& check) {
  if (!platform_support_->IsConnected())
    return;
  pending_checks_.push_back(check);
  std::vector<OverlayCheck_Params> list;
  for (const auto& candidate : candidates)
    list.push_back(OverlayCheck_Params(candidate));
  platform_support_->CheckOverlayCapabilities(widget_, list);
}

void DrmOverlayCandidatesHost::OnOverlayResult(bool* handled,
                                               gfx::AcceleratedWidget widget,
                                               bool result) {
  if (widget != widget_)
    return;
  *handled = true;
  DCHECK(!pending_checks_.empty());
  ProcessResult(pending_checks_.front(), result);
  pending_checks_.pop_front();
}

void DrmOverlayCandidatesHost::ProcessResult(const OverlayCheck_Params& check,
                                             bool result) {
  if (result)
    cache_.Put(check, true);
}

}  // namespace ui
