// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/gfx/compositor/graph/snapshot.h"

#include "base/logging.h"
#include "mojo/services/gfx/composition/cpp/formatting.h"
#include "mojo/skia/type_converters.h"
#include "services/gfx/compositor/graph/scene_content.h"
#include "third_party/skia/include/core/SkMatrix44.h"
#include "third_party/skia/include/core/SkPictureRecorder.h"
#include "third_party/skia/include/core/SkRect.h"

namespace compositor {

Snapshot::Snapshot() {}

Snapshot::~Snapshot() {}

bool Snapshot::HasDependency(
    const mojo::gfx::composition::SceneToken& scene_token) const {
  return dependencies_.find(scene_token.value) != dependencies_.end();
}

scoped_refptr<RenderFrame> Snapshot::Paint(
    const RenderFrame::Metadata& metadata,
    const mojo::Rect& viewport) const {
  DCHECK(!is_blocked());
  DCHECK(root_scene_content_);

  SkIRect sk_viewport = viewport.To<SkIRect>();

  SkPictureRecorder recorder;
  recorder.beginRecording(SkRect::Make(sk_viewport));
  root_scene_content_->Paint(this, recorder.getRecordingCanvas());
  return new RenderFrame(metadata, sk_viewport,
                         recorder.finishRecordingAsPicture());
}

void Snapshot::HitTest(const mojo::PointF& point,
                       mojo::gfx::composition::HitTestResult* result) const {
  DCHECK(result);
  DCHECK(!is_blocked());
  DCHECK(root_scene_content_);

  root_scene_content_->HitTest(this, point.To<SkPoint>(), SkMatrix44::I(),
                               &result->root);
}

bool Snapshot::IsNodeBlocked(const Node* node) const {
  DCHECK(!is_blocked());

  auto it = node_dispositions_.find(node);
  DCHECK(it != node_dispositions_.end());
  DCHECK(it->second == Disposition::kSuccess ||
         it->second == Disposition::kBlocked);
  return it->second == Disposition::kBlocked;
}

const SceneContent* Snapshot::GetResolvedSceneContent(
    const SceneNode* scene_node) const {
  DCHECK(!is_blocked());

  auto it = resolved_scene_contents_.find(scene_node);
  DCHECK(it != resolved_scene_contents_.end());
  return it->second.get();
}

SnapshotBuilder::SnapshotBuilder(std::ostream* block_log)
    : snapshot_(new Snapshot()), block_log_(block_log) {}

SnapshotBuilder::~SnapshotBuilder() {}

Snapshot::Disposition SnapshotBuilder::SnapshotNode(
    const Node* node,
    const SceneContent* content) {
  DCHECK(snapshot_);
  DCHECK(node);
  DCHECK(content);

  auto it = snapshot_->node_dispositions_.find(node);
  if (it != snapshot_->node_dispositions_.end())
    return it->second;

  Snapshot::Disposition disposition = node->RecordSnapshot(content, this);
  snapshot_->node_dispositions_[node] = disposition;
  return disposition;
}

Snapshot::Disposition SnapshotBuilder::SnapshotReferencedScene(
    const SceneNode* referrer_node,
    const SceneContent* referrer_content) {
  DCHECK(snapshot_);
  DCHECK(referrer_node);
  DCHECK(referrer_content);

  // This function should only ever be called once when snapshotting the
  // referring |SceneNode| at which point the result will be memoized
  // by |SnapshotNode| as usual so reentrance should not occur.
  DCHECK(snapshot_->resolved_scene_contents_.find(referrer_node) ==
         snapshot_->resolved_scene_contents_.end());

  auto scene_resource =
      static_cast<const SceneResource*>(referrer_content->GetResource(
          referrer_node->scene_resource_id(), Resource::Type::kScene));
  DCHECK(scene_resource);

  scoped_refptr<const SceneContent> content;
  Snapshot::Disposition disposition = AddDependencyResolveAndSnapshotScene(
      scene_resource->scene_token(), referrer_node->scene_version(), &content);

  if (disposition == Snapshot::Disposition::kSuccess) {
    snapshot_->resolved_scene_contents_[referrer_node] = content;
  } else if (disposition == Snapshot::Disposition::kBlocked) {
    if (block_log_) {
      *block_log_ << "Scene node's referenced scene is blocked: "
                  << referrer_node->FormattedLabel(referrer_content)
                  << ", referenced scene " << scene_resource->scene_token()
                  << ", version " << referrer_node->scene_version()
                  << std::endl;
    }
  }
  return disposition;
}

Snapshot::Disposition SnapshotBuilder::SnapshotSceneContent(
    const SceneContent* content) {
  DCHECK(snapshot_);
  DCHECK(content);

  const Node* root = content->GetRootNodeIfExists();
  if (!root) {
    if (block_log_) {
      *block_log_ << "Scene has no root node: " << content->FormattedLabel()
                  << std::endl;
    }
    return Snapshot::Disposition::kBlocked;
  }

  return SnapshotNode(root, content);
}

Snapshot::Disposition SnapshotBuilder::AddDependencyResolveAndSnapshotScene(
    const mojo::gfx::composition::SceneToken& scene_token,
    uint32_t version,
    scoped_refptr<const SceneContent>* out_content) {
  DCHECK(out_content);

  snapshot_->dependencies_.insert(scene_token.value);
  return ResolveAndSnapshotScene(scene_token, version, out_content);
}

scoped_refptr<const Snapshot> SnapshotBuilder::Build(
    const mojo::gfx::composition::SceneToken& scene_token,
    uint32_t version) {
  DCHECK(snapshot_);
  DCHECK(!snapshot_->root_scene_content_);

  scoped_refptr<const SceneContent> content;
  snapshot_->disposition_ =
      AddDependencyResolveAndSnapshotScene(scene_token, version, &content);

  if (!snapshot_->is_blocked()) {
    snapshot_->root_scene_content_ = content;
  } else {
    snapshot_->resolved_scene_contents_.clear();
    snapshot_->node_dispositions_.clear();
  }
  return std::move(snapshot_);
}

}  // namespace compositor
