// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_GFX_COMPOSITOR_GRAPH_SNAPSHOT_H_
#define SERVICES_GFX_COMPOSITOR_GRAPH_SNAPSHOT_H_

#include <iosfwd>
#include <unordered_map>
#include <unordered_set>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "mojo/services/geometry/interfaces/geometry.mojom.h"
#include "mojo/services/gfx/composition/interfaces/hit_tests.mojom.h"
#include "mojo/services/gfx/composition/interfaces/scheduling.mojom.h"
#include "services/gfx/compositor/render/render_frame.h"

namespace compositor {

class Node;
class SceneContent;
class SceneNode;

// Describes a single frame snapshot of the scene graph, sufficient for
// rendering and hit testing.  When the snapshot is made, all predicated and
// blocked scene nodes are evaluated to produce a final description of
// the content of the frame along with its dependencies.
//
// The snapshot holds a list of dependencies for the scenes whose state was
// originally used to produce it so that the snapshot can be invalidated
// whenever one of these scenes changes.  Note that the snapshot will contain
// a list of dependencies even when rendering is blocked, in which case
// the dependencies express the set of scenes which, if updated,
// might allow composition to be unblocked and make progress on a subsequent
// frame.
//
// Snapshot objects are not thread-safe since they have direct references to
// the scene graph definition.  However, the snapshot's frame is thread-safe
// and is intended to be sent to the backend rasterizer.
//
// Once fully constructed, instances of this class are immutable and
// reference counted so they may be bound to scene references in other scenes.
class Snapshot : public base::RefCounted<Snapshot> {
 public:
  // Describes the result of a snapshot operation.
  enum class Disposition {
    kSuccess,  // The snapshot was successful.
    kBlocked,  // The node was blocked from rendering.
    kCycle,    // The node was blocked due to a cycle, must unwind fully.
  };

  // Returns true if the snapshot is blocked from rendering.
  bool is_blocked() const { return disposition_ == Disposition::kBlocked; }

  // Gets the root scene content for the snapshot, or null if blocked.
  const SceneContent* root_scene_content() const {
    return root_scene_content_.get();
  }

  // Returns true if the snapshot has a dependency on content from the
  // specified scene.
  bool HasDependency(
      const mojo::gfx::composition::SceneToken& scene_token) const;

  // Paints the content of the snapshot to produce a frame to be rendered.
  // Only valid if |!is_blocked()|.
  scoped_refptr<RenderFrame> Paint(const RenderFrame::Metadata& metadata,
                                   const mojo::Rect& viewport) const;

  // Performs a hit test at the specified point, populating the result.
  // Only valid if |!is_blocked()|.
  void HitTest(const mojo::PointF& point,
               mojo::gfx::composition::HitTestResult* result) const;

  // Returns true if the specified node was blocked from rendering.
  // Only valid if |!is_blocked()|.
  bool IsNodeBlocked(const Node* node) const;

  // Gets the scene content which was resolved by following a scene node link.
  // Only valid if |!is_blocked()|.
  const SceneContent* GetResolvedSceneContent(
      const SceneNode* scene_node) const;

 private:
  friend class base::RefCounted<Snapshot>;
  friend class SnapshotBuilder;

  Snapshot();
  ~Snapshot();

  // Disposition of the snapshot as a whole.
  Disposition disposition_;

  // Just the set of dependent scene tokens.  Used for invalidation.
  std::unordered_set<uint32_t> dependencies_;

  // The root scene in the graph.
  // This reference together with |resolved_scenes| retains all of the
  // nodes used by the snapshot so that we can use bare pointers for nodes
  // and avoid excess reference counting overhead in other data structures.
  // Empty when the snapshot is blocked.
  scoped_refptr<const SceneContent> root_scene_content_;

  // Map of scenes which were resolved from scene nodes.
  // Empty when the snapshot is blocked.
  std::unordered_map<const SceneNode*, scoped_refptr<const SceneContent>>
      resolved_scene_contents_;

  // Node dispositions.  We only ever observe |kSuccess| or |kBlocked| here.
  // Empty when the snapshot is blocked.
  std::unordered_map<const Node*, Disposition> node_dispositions_;

  DISALLOW_COPY_AND_ASSIGN(Snapshot);
};

// Builds a table of all of the state which will be required for rendering
// a scene graph.
class SnapshotBuilder {
 public:
  explicit SnapshotBuilder(std::ostream* block_log);
  virtual ~SnapshotBuilder();

  // If not null, the snapshotter will append information to this stream
  // describing the parts of the scene graph for which composition was blocked.
  std::ostream* block_log() { return block_log_; }

  // Snapshots the requested node.
  Snapshot::Disposition SnapshotNode(const Node* node,
                                     const SceneContent* content);

  // Snapshots the referenced scene.
  Snapshot::Disposition SnapshotReferencedScene(
      const SceneNode* referrer_node,
      const SceneContent* referrer_content);

  // Builds a snapshot rooted at the specified scene.
  scoped_refptr<const Snapshot> Build(
      const mojo::gfx::composition::SceneToken& scene_token,
      uint32_t version);

 protected:
  // Resolves and snapshots a particular version of a scene.
  virtual Snapshot::Disposition ResolveAndSnapshotScene(
      const mojo::gfx::composition::SceneToken& scene_token,
      uint32_t version,
      scoped_refptr<const SceneContent>* out_content) = 0;

  // Snapshots a scene.
  Snapshot::Disposition SnapshotSceneContent(const SceneContent* content);

 private:
  Snapshot::Disposition AddDependencyResolveAndSnapshotScene(
      const mojo::gfx::composition::SceneToken& scene_token,
      uint32_t version,
      scoped_refptr<const SceneContent>* out_content);

  scoped_refptr<Snapshot> snapshot_;
  std::ostream* const block_log_;

  DISALLOW_COPY_AND_ASSIGN(SnapshotBuilder);
};

}  // namespace compositor

#endif  // SERVICES_GFX_COMPOSITOR_GRAPH_SNAPSHOT_H_
