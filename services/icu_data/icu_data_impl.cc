// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/icu_data/icu_data_impl.h"

#include "mojo/public/cpp/bindings/interface_ptr.h"
#include "services/icu_data/kICUData.h"

namespace icu_data {

ICUDataImpl::ICUDataImpl() {}
ICUDataImpl::~ICUDataImpl() {}

bool ICUDataImpl::OnAcceptConnection(
    mojo::ServiceProviderImpl* service_provider_impl) {
  service_provider_impl->AddService<ICUData>(
      [this](const mojo::ConnectionContext& connection_context,
             mojo::InterfaceRequest<ICUData> icu_data_request) {
        bindings_.AddBinding(this, icu_data_request.Pass());
      });

  return true;
}

void ICUDataImpl::Map(
    const mojo::String& sha1hash,
    const mojo::Callback<void(mojo::ScopedSharedBufferHandle)>& callback) {
  if (std::string(sha1hash) != std::string(kICUData.hash)) {
    LOG(WARNING) << "Failed to match sha1sum. Expected " << kICUData.hash;
    callback.Run(mojo::ScopedSharedBufferHandle());
    return;
  }
  EnsureBuffer();
  mojo::ScopedSharedBufferHandle handle;
  // FIXME: We should create a read-only duplicate of the handle.
  mojo::DuplicateBuffer(buffer_->handle.get(), nullptr, &handle);
  callback.Run(handle.Pass());
}

void ICUDataImpl::EnsureBuffer() {
  if (buffer_)
    return;
  buffer_.reset(new mojo::SharedBuffer(kICUData.size));
  void* ptr = nullptr;
  MojoResult rv = mojo::MapBuffer(buffer_->handle.get(), 0, kICUData.size, &ptr,
                                  MOJO_MAP_BUFFER_FLAG_NONE);
  CHECK_EQ(rv, MOJO_RESULT_OK);
  memcpy(ptr, kICUData.data, kICUData.size);
  rv = mojo::UnmapBuffer(ptr);
  CHECK_EQ(rv, MOJO_RESULT_OK);
}

}  // namespace icu_data
