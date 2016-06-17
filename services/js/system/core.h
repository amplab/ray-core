// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_JS_SYSTEM_CORE_H_
#define SERVICES_JS_SYSTEM_CORE_H_

#include "v8/include/v8.h"

namespace mojo {
namespace js {

class Core {
 public:
  static const char kModuleName[];
  static v8::Local<v8::Value> GetModule(v8::Isolate* isolate);
};

}  // namespace js
}  // namespace mojo

#endif  // SERVICES_JS_SYSTEM_CORE_H_
