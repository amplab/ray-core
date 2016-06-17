// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/nacl/nonsfi/nexe_launcher_nonsfi.h"

#include "base/files/file_util.h"
#include "mojo/nacl/nonsfi/irt_mojo_nonsfi.h"
#include "mojo/public/c/system/handle.h"
#include "native_client/src/public/irt_core.h"
#include "native_client/src/public/nonsfi/elf_loader.h"

namespace nacl {

void MojoLaunchNexeNonsfi(int nexe_fd, MojoHandle initial_handle,
                          bool enable_translate_irt) {
  // Run -- also, closes the nexe_fd, removing the temp file.
  uintptr_t entry = NaClLoadElfFile(nexe_fd);

  // Enable the translation section of the IRT, if requested.
  if (enable_translate_irt) {
    MojoPnaclTranslatorEnable();
  }
  MojoSetInitialHandle(initial_handle);
  int argc = 1;
  char* argvp = const_cast<char*>("NaClMain");
  char* envp = nullptr;
  nacl_irt_nonsfi_entry(argc, &argvp, &envp,
                        reinterpret_cast<nacl_entry_func_t>(entry),
                        MojoIrtNonsfiQuery);
  abort();
  NOTREACHED();
}

} // namespace nacl
