// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_VIEW_MANAGER_VIEW_STATE_H_
#define SERVICES_UI_VIEW_MANAGER_VIEW_STATE_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/services/ui/views/cpp/formatting.h"
#include "mojo/services/ui/views/interfaces/views.mojom.h"
#include "services/ui/view_manager/view_container_state.h"

namespace view_manager {

class ViewRegistry;
class ViewImpl;

// Describes the state of a particular view.
// This object is owned by the ViewRegistry that created it.
class ViewState : public ViewContainerState {
 public:
  enum {
    // View invalidated itself explicitly (wants to draw).
    INVALIDATION_EXPLICIT = 1u << 0,

    // Properties may have changed and must be resolved.
    INVALIDATION_PROPERTIES_CHANGED = 1u << 1,

    // View's parent changed, may require resolving properties.
    INVALIDATION_PARENT_CHANGED = 1u << 2,
  };

  ViewState(ViewRegistry* registry,
            mojo::ui::ViewTokenPtr view_token,
            mojo::InterfaceRequest<mojo::ui::View> view_request,
            mojo::ui::ViewListenerPtr view_listener,
            const std::string& label);
  ~ViewState() override;

  base::WeakPtr<ViewState> GetWeakPtr() { return weak_factory_.GetWeakPtr(); }

  // Gets the token used to refer to this view globally.
  // Caller does not obtain ownership of the token.
  const mojo::ui::ViewTokenPtr& view_token() const { return view_token_; }

  // Gets the view listener interface, never null.
  // Caller does not obtain ownership of the view listener.
  const mojo::ui::ViewListenerPtr& view_listener() const {
    return view_listener_;
  }

  // Gets or sets the view stub which links this view into the
  // view hierarchy, or null if the view isn't linked anywhere.
  ViewStub* view_stub() const { return view_stub_; }
  void set_view_stub(ViewStub* view_stub) { view_stub_ = view_stub; }

  // The current scene token, or null if none.
  const mojo::gfx::composition::SceneTokenPtr& scene_token() {
    return scene_token_;
  }
  void set_scene_token(mojo::gfx::composition::SceneTokenPtr scene_token) {
    scene_token_ = scene_token.Pass();
  }

  // Gets the scene version the view was asked to produce.
  // This value monotonically increases each time new properties are set.
  // This value is preserved across reparenting.
  uint32_t issued_scene_version() const { return issued_scene_version_; }

  // Gets the properties the view was asked to apply, after applying
  // any inherited properties from the container, or null if none set.
  // This value is preserved across reparenting.
  const mojo::ui::ViewPropertiesPtr& issued_properties() const {
    return issued_properties_;
  }

  // Sets the requested properties and increments the scene version.
  // Sets |issued_properties_valid()| to true if |properties| is not null.
  void IssueProperties(mojo::ui::ViewPropertiesPtr properties);

  // Gets or sets flags describing the invalidation state of the view.
  uint32_t invalidation_flags() const { return invalidation_flags_; }
  void set_invalidation_flags(uint32_t value) { invalidation_flags_ = value; }

  // Binds the |ViewOwner| interface to the view which has the effect of
  // tying the view's lifetime to that of the owner's pipe.
  void BindOwner(
      mojo::InterfaceRequest<mojo::ui::ViewOwner> view_owner_request);

  // Unbinds the view from its owner.
  void ReleaseOwner();

  ViewState* AsViewState() override;

  const std::string& label() const { return label_; }
  const std::string& FormattedLabel() const override;

 private:
  mojo::ui::ViewTokenPtr view_token_;
  mojo::ui::ViewListenerPtr view_listener_;

  const std::string label_;
  mutable std::string formatted_label_cache_;

  std::unique_ptr<ViewImpl> impl_;
  mojo::Binding<mojo::ui::View> view_binding_;
  mojo::Binding<mojo::ui::ViewOwner> owner_binding_;

  ViewStub* view_stub_ = nullptr;

  mojo::gfx::composition::SceneTokenPtr scene_token_;

  uint32_t issued_scene_version_ = 0u;
  mojo::ui::ViewPropertiesPtr issued_properties_;

  uint32_t invalidation_flags_ = 0u;

  base::WeakPtrFactory<ViewState> weak_factory_;  // must be last

  DISALLOW_COPY_AND_ASSIGN(ViewState);
};

}  // namespace view_manager

#endif  // SERVICES_UI_VIEW_MANAGER_VIEW_STATE_H_
