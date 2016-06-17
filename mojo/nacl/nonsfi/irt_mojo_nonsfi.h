// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_NACL_NONSFI_IRT_MOJO_NONSFI_H_
#define MOJO_NACL_NONSFI_IRT_MOJO_NONSFI_H_

#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/result.h"
#include "native_client/src/untrusted/irt/irt_dev.h"

namespace nacl {

extern const struct nacl_irt_private_pnacl_translator_compile
    nacl_irt_private_pnacl_translator_compile;
extern const struct nacl_irt_private_pnacl_translator_link
    nacl_irt_private_pnacl_translator_link;
extern const struct nacl_irt_resource_open
    nacl_irt_resource_open;

// Used to pass handle to application. If uncalled,
// the handle defaults to MOJO_HANDLE_INVALID.
void MojoSetInitialHandle(MojoHandle handle);

MojoResult MojoGetInitialHandle(MojoHandle* handle);

// Mechanism to control when nexes get access to the PNaCl translation
// IRT functions.
void MojoPnaclTranslatorEnable();

// IRT interface query function which may be passed to nacl_irt_nonsfi_entry.
// Queries for a mojo interface, then for the irt core.
size_t MojoIrtNonsfiQuery(const char* interface_ident,
                          void* table,
                          size_t tablesize);

}  // namespace nacl
#endif  // MOJO_NACL_NONSFI_IRT_MOJO_NONSFI_H_
