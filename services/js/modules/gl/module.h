// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_JS_MODULES_GL_MODULE_H_
#define SERVICES_JS_MODULES_GL_MODULE_H_

#include "gin/public/wrapper_info.h"
#include "v8/include/v8.h"

namespace js {
namespace gl {

class GL {
 public:
  static const char* kModuleName;
  static v8::Local<v8::Value> GetModule(v8::Isolate* isolate);
};

}  // namespace gl
}  // namespace js

#endif // SERVICES_JS_MODULES_GL_MODULE_H_

