// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_CRASH_CRASH_UPLOAD_H_
#define SHELL_CRASH_CRASH_UPLOAD_H_

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/task_runner.h"
#include "mojo/services/network/interfaces/network_service.mojom.h"

namespace breakpad {

// Upload a crash from dump_paths if more than one hour has passed since the
// last crash has been uploaded and clean up dumps_path.
void UploadCrashes(const base::FilePath& dumps_path,
                   scoped_refptr<base::TaskRunner> file_task_runner,
                   mojo::NetworkServicePtr network_service);

}  // namespace breakpad

#endif  // SHELL_CRASH_CRASH_UPLOAD_H_
