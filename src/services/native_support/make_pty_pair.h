// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NATIVE_SUPPORT_MAKE_PTY_PAIR_H_
#define SERVICES_NATIVE_SUPPORT_MAKE_PTY_PAIR_H_

#include <memory>
#include <vector>

#include "base/files/scoped_file.h"

namespace native_support {

// Makes a pseudoterminal master-slave pair of FDs. Returns true on success and
// false on failure in which case it sets |*errno_value| appropriately (and
// touches neither |master_fd| nor |slave_fd|).
bool MakePtyPair(base::ScopedFD* master_fd,
                 base::ScopedFD* slave_fd,
                 int* errno_value);

}  // namespace native_support

#endif  // SERVICES_NATIVE_SUPPORT_MAKE_PTY_PAIR_H_
