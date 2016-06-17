// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <errno.h>
#include <string.h>

#include "base/logging.h"
#include "mojo/nacl/nonsfi/file_util.h"
#include "mojo/nacl/nonsfi/irt_mojo_nonsfi.h"
#include "native_client/src/untrusted/irt/irt_dev.h"
#include "services/nacl/nonsfi/kCrtbegin.h"
#include "services/nacl/nonsfi/kCrtbeginForEh.h"
#include "services/nacl/nonsfi/kCrtend.h"
#include "services/nacl/nonsfi/kLibcrtPlatform.h"
#include "services/nacl/nonsfi/kLibgcc.h"
#include "services/nacl/nonsfi/kLibpnaclIrtShimDummy.h"

namespace {

const struct {
  const char* filename;
  const mojo::embed::Data* data;
} g_files[] = {
  {"crtbegin.o", &nacl::kCrtbegin},
  {"crtbegin_for_eh.o", &nacl::kCrtbeginForEh},
  {"crtend.o", &nacl::kCrtend},
  {"libcrt_platform.a", &nacl::kLibcrtPlatform},
  {"libgcc.a", &nacl::kLibgcc},
  {"libpnacl_irt_shim.a", &nacl::kLibpnaclIrtShimDummy},
};

int IrtOpenResource(const char* filename, int* newfd) {
  const mojo::embed::Data* data = nullptr;
  for (size_t i = 0; i < arraysize(g_files); i++) {
    if (!strcmp(filename, g_files[i].filename)) {
      data = g_files[i].data;
      break;
    }
  }
  CHECK(data) << "Could not find file: " << filename;
  int fd = nacl::DataToTempFileDescriptor(*data);
  if (fd < 0)
    return errno;
  *newfd = fd;
  return 0;
}

}  // namespace anonymous

namespace nacl {

const struct nacl_irt_resource_open nacl_irt_resource_open = {
  IrtOpenResource,
};

}  // namespace nacl
