// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_APPLICATION_MANAGER_SHELL_IMPL_H_
#define SHELL_APPLICATION_MANAGER_SHELL_IMPL_H_

#include "base/callback.h"
#include "base/macros.h"
#include "mojo/common/binding_set.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/interfaces/application/application.mojom.h"
#include "mojo/public/interfaces/application/application_connector.mojom.h"
#include "mojo/public/interfaces/application/shell.mojom.h"
#include "shell/application_manager/identity.h"
#include "url/gurl.h"

namespace shell {

class ApplicationManager;

// TODO(vtl): This both implements the |Shell| interface and holds the
// |ApplicationPtr| (from back when they were paired interfaces on the same
// message pipe). The way it manages lifetime is dubious/wrong. We should have
// an object holding stuff associated to an application, namely its "real"
// |ShellImpl|, its |ApplicationPtr|, and tracking its |ApplicationConnector|s.
class ShellImpl : public mojo::Shell {
 public:
  ShellImpl(mojo::ApplicationPtr application,
            ApplicationManager* manager,
            const Identity& resolved_identity,
            const base::Closure& on_application_end);

  ~ShellImpl() override;

  void InitializeApplication(mojo::Array<mojo::String> args);

  void ConnectToClient(const GURL& requested_url,
                       const GURL& requestor_url,
                       mojo::InterfaceRequest<mojo::ServiceProvider> services);

  mojo::Application* application() { return application_.get(); }
  const Identity& identity() const { return identity_; }
  base::Closure on_application_end() const { return on_application_end_; }

 private:
  // This is a per-|ShellImpl| singleton.
  class ApplicationConnectorImpl : public mojo::ApplicationConnector {
   public:
    explicit ApplicationConnectorImpl(mojo::Shell* shell);
    ~ApplicationConnectorImpl() override;

    void ConnectToApplication(
        const mojo::String& app_url,
        mojo::InterfaceRequest<mojo::ServiceProvider> services) override;
    void Duplicate(mojo::InterfaceRequest<mojo::ApplicationConnector>
                       application_connector_request) override;

   private:
    mojo::Shell* const shell_;
    mojo::BindingSet<mojo::ApplicationConnector> bindings_;

    DISALLOW_COPY_AND_ASSIGN(ApplicationConnectorImpl);
  };

  // mojo::Shell implementation:
  void ConnectToApplication(
      const mojo::String& app_url,
      mojo::InterfaceRequest<mojo::ServiceProvider> services) override;
  void CreateApplicationConnector(
      mojo::InterfaceRequest<mojo::ApplicationConnector>
          application_connector_request) override;

  ApplicationManager* const manager_;
  const Identity identity_;
  base::Closure on_application_end_;
  mojo::ApplicationPtr application_;
  mojo::Binding<mojo::Shell> binding_;

  ApplicationConnectorImpl application_connector_impl_;

  DISALLOW_COPY_AND_ASSIGN(ShellImpl);
};

}  // namespace shell

#endif  // SHELL_APPLICATION_MANAGER_SHELL_IMPL_H_
