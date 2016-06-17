// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_GLES2_MAILBOX_MANAGER_FACTORY_H_
#define SERVICES_GLES2_MAILBOX_MANAGER_FACTORY_H_

#include "gpu/command_buffer/service/mailbox_manager.h"

namespace gles2 {

class MailboxManagerFactory {
 public:
  static gpu::gles2::MailboxManager* Create();
};

}  // namespace gles2

#endif  // SERVICES_GLES2_MAILBOX_MANAGER_FACTORY_H_
