// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "mojo/common/binding_set.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/services/icu_data/interfaces/icu_data.mojom.h"

namespace icu_data {

class ICUDataImpl : public mojo::ApplicationImplBase, public ICUData {
 public:
  ICUDataImpl();
  ~ICUDataImpl() override;

  // mojo::ApplicationImplBase implementation.
  bool OnAcceptConnection(
      mojo::ServiceProviderImpl* service_provider_impl) override;

  void Map(const mojo::String& sha1hash,
           const mojo::Callback<void(mojo::ScopedSharedBufferHandle)>& callback)
      override;

 private:
  void EnsureBuffer();

  scoped_ptr<mojo::SharedBuffer> buffer_;
  mojo::BindingSet<ICUData> bindings_;

  DISALLOW_COPY_AND_ASSIGN(ICUDataImpl);
};

}  // namespace icu_data;
