// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/ui/input_handler.h"

#include "base/logging.h"
#include "mojo/public/cpp/application/connect.h"

namespace mojo {
namespace ui {

InputHandler::InputHandler(ServiceProvider* service_provider,
                           InputListener* listener)
    : listener_binding_(listener) {
  DCHECK(service_provider);
  DCHECK(listener);

  ConnectToService(service_provider, GetProxy(&connection_));

  InputListenerPtr listener_ptr;
  listener_binding_.Bind(GetProxy(&listener_ptr));
  connection_->SetListener(listener_ptr.Pass());
}

InputHandler::~InputHandler() {}

}  // namespace ui
}  // namespace mojo
