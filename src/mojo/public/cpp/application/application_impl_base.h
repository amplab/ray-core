// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_APPLICATION_APPLICATION_IMPL_BASE_H_
#define MOJO_PUBLIC_CPP_APPLICATION_APPLICATION_IMPL_BASE_H_

#include <memory>
#include <string>
#include <vector>

#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/public/interfaces/application/application.mojom.h"
#include "mojo/public/interfaces/application/shell.mojom.h"

namespace mojo {

class ServiceProviderImpl;

// Base helper class for implementing the |Application| interface, which the
// shell uses for basic communication with an application (e.g., to connect
// clients to services provided by an application).
//
// To use this class, subclass it and implement/override the required methods
// (see below).
//
// Note that by default |ApplicationImplBase|s are not "strongly bound" to their
// |Application| requests (so, e.g., they can thus be constructed on the stack).
// A subclass may make itself strongly bound by setting a suitable connection
// error handler on the binding (available via |application_binding()|).
//
// The |RunApplication()| helper function (declared in run_application.h) takes
// a pointer to an instance of (a subclass of) |ApplicationImplBase| and runs it
// using a suitable message loop; see run_application.h for more details.
class ApplicationImplBase : public Application {
 public:
  ~ApplicationImplBase() override;

  // Binds the given |Application| request to this object. This must be done
  // with the message (run) loop available/running, and this will cause this
  // object to start serving requests (via that message loop).
  void Bind(InterfaceRequest<Application> application_request);

  // This will be valid after |Initialize()| has been received and remain valid
  // until this object is destroyed.
  Shell* shell() const { return shell_.get(); }

  // Returns this object's |Application| binding (set up by |Bind()|; of course,
  // you can always manipulate it directly).
  const Binding<Application>& application_binding() const {
    return application_binding_;
  }
  Binding<Application>& application_binding() { return application_binding_; }

  // Returns any initial configuration arguments, passed by the shell.
  const std::vector<std::string>& args() const { return args_; }
  bool HasArg(const std::string& arg) const;

  const std::string& url() const { return url_; }

  // Methods to be overridden (if desired) by subclasses:

  // Called after |Initialize()| has been received (|shell()|, |args()|, and
  // |url()| will be valid when this is called. The default implementation does
  // nothing.
  virtual void OnInitialize();

  // Called when another application connects to this application (i.e., we
  // receive |AcceptConnection()|). This should either configure what services
  // are "provided" (made available via a |ServiceProvider|) to that application
  // and return true, or this may return false to reject the connection
  // entirely. The default implementation rejects all connections (by just
  // returning false).
  virtual bool OnAcceptConnection(ServiceProviderImpl* service_provider_impl);

  // Called before quitting the main message (run) loop, i.e., before
  // |Terminate()|. The default implementation does nothing.
  virtual void OnQuit();

  // Terminates this application with result |result|. The default
  // implementation quits the main run loop for this application by calling
  // |mojo::TerminateApplication()| (and assumes that the application was run
  // using |mojo::RunApplication()|).
  virtual void Terminate(MojoResult result);

 protected:
  // This class is meant to be subclassed.
  ApplicationImplBase();

 private:
  // |Application| implementation. In general, you probably shouldn't call these
  // directly (but I can't really stop you).
  void Initialize(InterfaceHandle<Shell> shell,
                  Array<String> args,
                  const mojo::String& url) final;
  void AcceptConnection(const String& requestor_url,
                        const String& url,
                        InterfaceRequest<ServiceProvider> services) final;
  void RequestQuit() final;

  Binding<Application> application_binding_;

  // Set by |Initialize()|.
  ShellPtr shell_;
  std::vector<std::string> args_;
  std::string url_;

  std::vector<std::unique_ptr<ServiceProviderImpl>> service_provider_impls_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(ApplicationImplBase);
};

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_APPLICATION_APPLICATION_IMPL_BASE_H_
