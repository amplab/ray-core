// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/prediction/prediction_service_impl.h"

#include "mojo/environment/scoped_chromium_init.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/application/service_provider_impl.h"

namespace prediction {

PredictionServiceImpl::PredictionServiceImpl(
    mojo::InterfaceRequest<PredictionService> request)
    : strong_binding_(this, request.Pass()) {
  ProximityInfoFactory proximity_info;
  proximity_settings_ = scoped_ptr<latinime::ProximityInfo>(
      proximity_info.GetNativeProximityInfo());
}

PredictionServiceImpl::~PredictionServiceImpl() {
}

void PredictionServiceImpl::GetPredictionList(
    PredictionInfoPtr prediction_info,
    const GetPredictionListCallback& callback) {
  mojo::Array<mojo::String> prediction_list =
      dictionary_service_.GetDictionarySuggestion(prediction_info.Pass(),
                                                  proximity_settings_.get());
  callback.Run(prediction_list.Pass());
}

// TODO(vtl): Maybe this (and MojoMain()) should go in a different file?
class PredictionServiceApp : public mojo::ApplicationImplBase {
 public:
  PredictionServiceApp() {}
  ~PredictionServiceApp() override {}

  // mojo::ApplicationImplBase override:
  bool OnAcceptConnection(
      mojo::ServiceProviderImpl* service_provider_impl) override {
    service_provider_impl->AddService<PredictionService>([](
        const mojo::ConnectionContext& connection_context,
        mojo::InterfaceRequest<PredictionService> prediction_service_request) {
      new PredictionServiceImpl(prediction_service_request.Pass());
    });
    return true;
  }
};

}  // namespace prediction

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ScopedChromiumInit init;
  prediction::PredictionServiceApp prediction_service_app;
  return mojo::RunApplication(application_request, &prediction_service_app);
}
