// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_DART_EMBEDDER_MOJO_IO_NATIVES_H_
#define MOJO_DART_EMBEDDER_MOJO_IO_NATIVES_H_

#include "dart/runtime/include/dart_api.h"

namespace mojo {
namespace dart {

Dart_NativeFunction MojoIoNativeLookup(Dart_Handle name,
                                     int argument_count,
                                     bool* auto_setup_scope);

const uint8_t* MojoIoNativeSymbol(Dart_NativeFunction nf);

}  // namespace dart
}  // namespace mojo

#endif  // MOJO_DART_EMBEDDER_MOJO_IO_NATIVES_H_
