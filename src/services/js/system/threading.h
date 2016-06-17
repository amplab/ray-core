// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_JS_SYSTEM_THREADING_H_
#define SERVICES_JS_SYSTEM_THREADING_H_

#include "gin/public/wrapper_info.h"
#include "v8/include/v8.h"

namespace mojo {
namespace js {

class Threading {
 public:
  static const char kModuleName[];
  static v8::Local<v8::Value> GetModule(v8::Isolate* isolate);
 private:
  Threading();
};

}  // namespace js
}  // namespace mojo

#endif  // SERVICES_JS_SYSTEM_THREADING_H_
