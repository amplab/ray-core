// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_VIEW_MANAGER_VIEW_STUB_H_
#define SERVICES_UI_VIEW_MANAGER_VIEW_STUB_H_

#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "mojo/services/ui/views/interfaces/views.mojom.h"

namespace view_manager {

class ViewContainerState;
class ViewRegistry;
class ViewState;
class ViewTreeState;

// Describes a link in the view hierarchy either from a parent view to one
// of its children or from the view tree to its root view.
//
// When this object is created, it is not yet known whether the linked
// view actually exists.  We must wait for a response from the view owner
// to resolve the view's token and associate the stub with its child.
//
// Instances of this object are held by a unique pointer owned by the
// parent view or view tree at the point where the view is being linked.
// Note that the lifetime of the views themselves is managed by the view
// registry.
class ViewStub {
 public:
  // Begins the process of resolving a view.
  // Invokes |ViewRegistry.OnViewResolved| when the token is obtained
  // from the owner or passes nullptr if an error occurs.
  ViewStub(ViewRegistry* registry,
           mojo::InterfaceHandle<mojo::ui::ViewOwner> owner);
  ~ViewStub();

  base::WeakPtr<ViewStub> GetWeakPtr() { return weak_factory_.GetWeakPtr(); }

  // Gets the view state referenced by the stub, or null if the view
  // has not yet been resolved or is unavailable.
  ViewState* state() const { return state_; }

  // Returns true if the view which was intended to be referenced by the
  // stub has become unavailable.
  bool is_unavailable() const { return unavailable_; }

  // Returns true if awaiting resolution of the view.
  bool is_pending() const { return !state_ && !unavailable_; }

  // Returns true if the view is linked into a tree or parent.
  bool is_linked() const { return tree_ || parent_; }

  // Returns true if the view is linked into a tree and has no parent.
  bool is_root_of_tree() const { return tree_ && !parent_; }

  // Gets the view tree to which this view belongs, or null if none.
  ViewTreeState* tree() const { return tree_; }

  // Gets the parent view state, or null if none.
  ViewState* parent() const { return parent_; }

  // Gets the container, or null if none.
  ViewContainerState* container() const;

  // Gets the key that this child has in its container, or 0 if none.
  uint32_t key() const { return key_; }

  // Gets the wrapped scene exposed to the container.
  const mojo::gfx::composition::ScenePtr& stub_scene() const {
    return stub_scene_;
  }
  const mojo::gfx::composition::SceneTokenPtr& stub_scene_token() const {
    return stub_scene_token_;
  }

  // Gets the scene version which the container set on this view, or 0
  // if none set or the view has become unavailable.
  uint32_t scene_version() const { return scene_version_; }

  // Gets the properties which the container set on this view, or null
  // if none set or the view has become unavailable.
  const mojo::ui::ViewPropertiesPtr& properties() const { return properties_; }

  // Sets the scene version and properties set by the container.
  // May be called when the view is pending or attached but not after it
  // has become unavailable.
  void SetProperties(uint32_t scene_version,
                     mojo::ui::ViewPropertiesPtr properties);

  // Binds the stub to the specified actual view, which must not be null.
  // Must be called at most once to apply the effects of resolving the
  // view owner.
  void AttachView(ViewState* state,
                  mojo::gfx::composition::ScenePtr stub_scene);

  // Sets the stub scene token, which must not be null.
  // Called after |AttachView| once the scene token is known but the view
  // must not have been released.
  void SetStubSceneToken(
      mojo::gfx::composition::SceneTokenPtr stub_scene_token);

  // Marks the stub as unavailable.
  // Returns the previous view state, or null if none.
  ViewState* ReleaseView();

  // THESE METHODS SHOULD ONLY BE CALLED BY VIEW STATE OR VIEW TREE STATE

  // Sets the child's container and key.  Must not be null.
  void SetContainer(ViewContainerState* container, uint32_t key);

  // Resets the parent view state and tree pointers to null.
  void Unlink();

 private:
  void SetTreeRecursively(ViewTreeState* tree);
  static void SetTreeForChildrenOfView(ViewState* view, ViewTreeState* tree);

  void OnViewResolved(mojo::ui::ViewTokenPtr view_token);

  ViewRegistry* registry_;
  mojo::ui::ViewOwnerPtr owner_;
  ViewState* state_ = nullptr;
  bool unavailable_ = false;
  mojo::gfx::composition::ScenePtr stub_scene_;
  mojo::gfx::composition::SceneTokenPtr stub_scene_token_;

  uint32_t scene_version_ = mojo::gfx::composition::kSceneVersionNone;
  mojo::ui::ViewPropertiesPtr properties_;

  ViewTreeState* tree_ = nullptr;
  ViewState* parent_ = nullptr;
  uint32_t key_ = 0u;

  base::WeakPtrFactory<ViewStub> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ViewStub);
};

}  // namespace view_manager

#endif  // SERVICES_UI_VIEW_MANAGER_VIEW_STUB_H_
