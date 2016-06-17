// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/view_manager/view_state.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "services/ui/view_manager/view_impl.h"
#include "services/ui/view_manager/view_registry.h"
#include "services/ui/view_manager/view_stub.h"

namespace view_manager {

ViewState::ViewState(ViewRegistry* registry,
                     mojo::ui::ViewTokenPtr view_token,
                     mojo::InterfaceRequest<mojo::ui::View> view_request,
                     mojo::ui::ViewListenerPtr view_listener,
                     const std::string& label)
    : view_token_(view_token.Pass()),
      view_listener_(view_listener.Pass()),
      label_(label),
      impl_(new ViewImpl(registry, this)),
      view_binding_(impl_.get(), view_request.Pass()),
      owner_binding_(impl_.get()),
      weak_factory_(this) {
  DCHECK(view_token_);
  DCHECK(view_listener_);

  view_binding_.set_connection_error_handler(
      base::Bind(&ViewRegistry::OnViewDied, base::Unretained(registry),
                 base::Unretained(this), "View connection closed"));
  owner_binding_.set_connection_error_handler(
      base::Bind(&ViewRegistry::OnViewDied, base::Unretained(registry),
                 base::Unretained(this), "ViewOwner connection closed"));
  view_listener_.set_connection_error_handler(
      base::Bind(&ViewRegistry::OnViewDied, base::Unretained(registry),
                 base::Unretained(this), "ViewListener connection closed"));
}

ViewState::~ViewState() {}

void ViewState::IssueProperties(mojo::ui::ViewPropertiesPtr properties) {
  issued_scene_version_++;
  CHECK(issued_scene_version_);
  issued_properties_ = properties.Pass();
}

void ViewState::BindOwner(
    mojo::InterfaceRequest<mojo::ui::ViewOwner> view_owner_request) {
  DCHECK(!owner_binding_.is_bound());
  owner_binding_.Bind(view_owner_request.Pass());
}

void ViewState::ReleaseOwner() {
  DCHECK(owner_binding_.is_bound());
  owner_binding_.Close();
}

ViewState* ViewState::AsViewState() {
  return this;
}

const std::string& ViewState::FormattedLabel() const {
  if (formatted_label_cache_.empty()) {
    formatted_label_cache_ =
        label_.empty() ? base::StringPrintf("<V%d>", view_token_->value)
                       : base::StringPrintf("<V%d:%s>", view_token_->value,
                                            label_.c_str());
  }
  return formatted_label_cache_;
}

std::ostream& operator<<(std::ostream& os, ViewState* view_state) {
  if (!view_state)
    return os << "null";
  return os << view_state->FormattedLabel();
}

}  // namespace view_manager
