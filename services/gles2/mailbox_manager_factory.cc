// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "services/gles2/mailbox_manager_factory.h"

#include "gpu/command_buffer/service/mailbox_manager_impl.h"
#include "gpu/command_buffer/service/mailbox_manager_sync.h"

namespace gles2 {

gpu::gles2::MailboxManager* MailboxManagerFactory::Create() {
#if defined(OS_ANDROID)
  // Use egl fences for synchronization of textures across contexts on android.
  // TODO(cstout): see if enabling context virtualization removes the need for
  // fences.
  return new gpu::gles2::MailboxManagerSync;
#else
  return new gpu::gles2::MailboxManagerImpl;
#endif
}

}  // namespace gles2
