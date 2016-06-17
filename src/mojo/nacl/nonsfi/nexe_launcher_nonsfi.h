// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_NACL_NONSFI_NEXE_LAUNCHER_NONSFI_H_
#define MOJO_NACL_NONSFI_NEXE_LAUNCHER_NONSFI_H_

#include "mojo/public/c/system/handle.h"

namespace nacl {

/**
 * Takes a fd to a nexe, and launches the nexe with the given
 * MojoHandle.
 */
void MojoLaunchNexeNonsfi(int nexe_fd, MojoHandle initial_handle,
                          bool enable_translate_irt);

} // namespace nacl

#endif  // MOJO_NACL_NONSFI_NEXE_LAUNCHER_NONSFI_H_
