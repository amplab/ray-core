// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is a minimal definition of |Environment| that includes only a logger.
// This is just enough to be able to use |MOJO_LOG()| and related macros in
// logging.h.

#include "mojo/public/cpp/environment/environment.h"

#include "mojo/public/cpp/environment/lib/default_logger.h"

namespace mojo {

// static
const MojoLogger* Environment::GetDefaultLogger() {
  return &internal::kDefaultLogger;
}

}  // namespace mojo
