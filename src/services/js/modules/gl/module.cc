// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/js/modules/gl/module.h"

#include "gin/arguments.h"
#include "gin/object_template_builder.h"
#include "gin/per_isolate_data.h"
#include "gin/wrappable.h"
#include "services/js/modules/gl/context.h"
#include "services/js/system/handle.h"

namespace js {
namespace gl {

namespace {

gin::WrapperInfo kWrapperInfo = { gin::kEmbedderNativeGin };

gin::Handle<Context> CreateContext(
    const gin::Arguments& args,
    mojo::Handle handle,
    v8::Handle<v8::Function> context_lost_callback) {
  return Context::Create(args.isolate(), handle, context_lost_callback);
}

}  // namespace

const char* GL::kModuleName = "services/js/modules/gl";

v8::Local<v8::Value> GL::GetModule(v8::Isolate* isolate) {
  gin::PerIsolateData* data = gin::PerIsolateData::From(isolate);
  v8::Local<v8::ObjectTemplate> templ = data->GetObjectTemplate(&kWrapperInfo);

  if (templ.IsEmpty()) {
    templ = gin::ObjectTemplateBuilder(isolate)
        .SetMethod("Context", CreateContext)
        .Build();
    data->SetObjectTemplate(&kWrapperInfo, templ);
  }

  return templ->NewInstance();
}

}  // namespace gl
}  // namespace js
