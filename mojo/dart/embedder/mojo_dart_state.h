// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_DART_EMBEDDER_DART_STATE_H_
#define MOJO_DART_EMBEDDER_DART_STATE_H_

#include <set>
#include <string>

#include "base/callback.h"
#include "base/message_loop/message_loop.h"
#include "mojo/public/c/system/handle.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "mojo/services/network/interfaces/network_service.mojom.h"
#include "tonic/dart_library_provider.h"
#include "tonic/dart_state.h"

namespace mojo {
namespace dart {

struct IsolateCallbacks {
  base::Callback<void(Dart_Handle, int64_t)> exception;
};

// State associated with an isolate (retrieved via |Dart_CurrentIsolateData|).
class MojoDartState : public tonic::DartState {
 public:
  MojoDartState(void* application_data,
                IsolateCallbacks callbacks,
                std::string script_uri,
                std::string base_uri,
                std::string package_root,
                bool use_network_loader,
                bool use_dart_run_loop)
      : application_data_(application_data),
        callbacks_(callbacks),
        script_uri_(script_uri),
        base_uri_(base_uri),
        package_root_(package_root),
        library_provider_(nullptr),
        use_network_loader_(use_network_loader),
        use_dart_run_loop_(use_dart_run_loop) {
  }

  void* application_data() const { return application_data_; }
  const IsolateCallbacks& callbacks() const { return callbacks_; }
  const std::string& script_uri() const { return script_uri_; }
  const std::string& base_uri() const { return base_uri_; }
  const std::string& package_root() const { return package_root_; }

  bool use_network_loader() const { return use_network_loader_; }
  bool use_dart_run_loop() const { return use_dart_run_loop_; }

  void set_library_provider(tonic::DartLibraryProvider* library_provider) {
    library_provider_.reset(library_provider);
    DCHECK(library_provider_.get() == library_provider);
  }

  // Takes ownership of |raw_handle|.
  void BindNetworkService(MojoHandle raw_handle) {
    if (raw_handle == MOJO_HANDLE_INVALID) {
      return;
    }
    DCHECK(!network_service_.is_bound());
    MessagePipeHandle handle(raw_handle);
    ScopedMessagePipeHandle message_pipe(handle);
    InterfaceHandle<mojo::NetworkService> interface_info(message_pipe.Pass(),
                                                         0);
    network_service_.Bind(interface_info.Pass());
    DCHECK(network_service_.is_bound());
  }

  mojo::NetworkService* network_service() {
    // Should only be called after |BindNetworkService|.
    DCHECK(network_service_.is_bound());
    return network_service_.get();
  }

  tonic::DartLibraryProvider* library_provider() const {
    return library_provider_.get();
  }

  static MojoDartState* From(Dart_Isolate isolate) {
    return reinterpret_cast<MojoDartState*>(DartState::From(isolate));
  }

  static MojoDartState* Current() {
    return reinterpret_cast<MojoDartState*>(DartState::Current());
  }

  static MojoDartState* Cast(void* data) {
    return reinterpret_cast<MojoDartState*>(data);
  }

 private:
  void* application_data_;
  IsolateCallbacks callbacks_;
  std::string script_uri_;
  std::string base_uri_;
  std::string package_root_;
  std::unique_ptr<tonic::DartLibraryProvider> library_provider_;
  mojo::NetworkServicePtr network_service_;
  bool use_network_loader_;
  bool use_dart_run_loop_;
};

}  // namespace dart
}  // namespace mojo

#endif  // MOJO_DART_EMBEDDER_DART_STATE_H_
