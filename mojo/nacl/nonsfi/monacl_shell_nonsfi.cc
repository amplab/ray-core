// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fcntl.h>
#include <string.h>

#include <iostream>

#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/simple_platform_support.h"
#include "mojo/nacl/nonsfi/irt_mojo_nonsfi.h"
#include "native_client/src/public/irt_core.h"
#include "native_client/src/public/nonsfi/elf_loader.h"

int main(int argc, char** argv, char** environ) {
  nacl_irt_nonsfi_allow_dev_interfaces();
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] <<  " <executable> <args...>\n";
    return 1;
  }
  const char* nexe_filename = argv[1];
  int fd = open(nexe_filename, O_RDONLY);
  if (fd < 0) {
    std::cerr << "Failed to open " << nexe_filename << ": " <<
              strerror(errno) << "\n";
    return 1;
  }
  uintptr_t entry = NaClLoadElfFile(fd);

  mojo::embedder::Init(mojo::embedder::CreateSimplePlatformSupport());

  return nacl_irt_nonsfi_entry(argc - 1, argv + 1, environ,
                               reinterpret_cast<nacl_entry_func_t>(entry),
                               nacl::MojoIrtNonsfiQuery);
}
