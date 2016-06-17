// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DART_CONTENT_HANDLER_APP_SERVICE_CONNECTOR_H_
#define SERVICES_DART_CONTENT_HANDLER_APP_SERVICE_CONNECTOR_H_

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "mojo/dart/embedder/dart_controller.h"

namespace mojo {
class Shell;
}  // namespace mojo

namespace dart {

// Instances of this class must be constructed on the Dart content handler
// message loop thread.
class ContentHandlerAppServiceConnector
    : public mojo::dart::DartControllerServiceConnector {
 public:
  // Call this only on the same thread as the content_handler_runner. This is
  // checked in the body of the constructor.
  // TODO(vtl): Maybe this should take an
  // |InterfaceHandle<ApplicationConnector>| instead of a |Shell*|.
  explicit ContentHandlerAppServiceConnector(mojo::Shell* shell);

  // Must be called on the same thread that constructed |this|.
  ~ContentHandlerAppServiceConnector() override;

  // Implementing the DartControllerServiceConnector interface.
  // This is the only method that can be called from any thread.
  MojoHandle ConnectToService(ServiceId service_id) override;

 private:
  template<typename Interface>
  void Connect(std::string application_name,
               mojo::InterfaceRequest<Interface> interface_request);

  scoped_refptr<base::SingleThreadTaskRunner> runner_;
  mojo::Shell* shell_;
  base::WeakPtrFactory<ContentHandlerAppServiceConnector> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ContentHandlerAppServiceConnector);
};


}  // namespace dart

#endif  // SERVICES_DART_CONTENT_HANDLER_APP_SERVICE_CONNECTOR_H_
