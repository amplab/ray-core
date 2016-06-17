// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_APPLICATION_MANAGER_APPLICATION_MANAGER_H_
#define SHELL_APPLICATION_MANAGER_APPLICATION_MANAGER_H_

#include <map>

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/interfaces/application/application.mojom.h"
#include "mojo/public/interfaces/application/service_provider.mojom.h"
#include "mojo/services/network/interfaces/network_service.mojom.h"
#include "mojo/services/url_response_disk_cache/interfaces/url_response_disk_cache.mojom.h"
#include "shell/application_manager/application_loader.h"
#include "shell/application_manager/identity.h"
#include "shell/application_manager/native_application_options.h"
#include "shell/application_manager/native_runner.h"
#include "shell/native_application_support.h"
#include "url/gurl.h"

namespace base {
class FilePath;
class SequencedWorkerPool;
}

namespace shell {

class Fetcher;
class ShellImpl;

class ApplicationManager {
 public:
  struct Options {
    Options() : disable_cache(false), force_offline_by_default(false) {}

    bool disable_cache;
    bool force_offline_by_default;
  };

  class Delegate {
   public:
    // Gives the delegate a chance to apply any mappings for the specified url.
    // This should not resolve 'mojo' urls, that is done by ResolveMojoURL().
    virtual GURL ResolveMappings(const GURL& url) = 0;

    // Used to map a url with the scheme 'mojo' to the appropriate url. Return
    // |url| if the scheme is not 'mojo'.
    virtual GURL ResolveMojoURL(const GURL& url) = 0;

   protected:
    virtual ~Delegate() {}
  };

  // API for testing.
  class TestAPI {
   public:
    explicit TestAPI(ApplicationManager* manager);
    ~TestAPI();

    // Returns true if the shared instance has been created.
    static bool HasCreatedInstance();
    // Returns true if there is a ShellImpl for this URL.
    bool HasFactoryForURL(const GURL& url) const;

   private:
    ApplicationManager* manager_;

    DISALLOW_COPY_AND_ASSIGN(TestAPI);
  };

  ApplicationManager(const Options& options, Delegate* delegate);
  ~ApplicationManager();

  // Loads a service if necessary and establishes a new client connection.
  void ConnectToApplication(
      const GURL& application_url,
      const GURL& requestor_url,
      mojo::InterfaceRequest<mojo::ServiceProvider> services,
      const base::Closure& on_application_end);

  template <typename Interface>
  inline void ConnectToService(const GURL& application_url,
                               mojo::InterfacePtr<Interface>* ptr) {
    mojo::ScopedMessagePipeHandle service_handle =
        ConnectToServiceByName(application_url, Interface::Name_);
    ptr->Bind(mojo::InterfaceHandle<Interface>(service_handle.Pass(), 0u));
  }

  mojo::ScopedMessagePipeHandle ConnectToServiceByName(
      const GURL& application_url,
      const std::string& interface_name);

  void RegisterContentHandler(const std::string& mime_type,
                              const GURL& content_handler_url);

  // Sets the default Loader to be used if not overridden by SetLoaderForURL()
  // or SetLoaderForScheme().
  void set_default_loader(scoped_ptr<ApplicationLoader> loader) {
    default_loader_ = loader.Pass();
  }
  void set_native_runner_factory(
      scoped_ptr<NativeRunnerFactory> runner_factory) {
    native_runner_factory_ = runner_factory.Pass();
  }
  void set_blocking_pool(base::SequencedWorkerPool* blocking_pool) {
    blocking_pool_ = blocking_pool;
  }
  // Sets a Loader to be used for a specific url.
  void SetLoaderForURL(scoped_ptr<ApplicationLoader> loader, const GURL& url);
  // Sets a Loader to be used for a specific url scheme.
  void SetLoaderForScheme(scoped_ptr<ApplicationLoader> loader,
                          const std::string& scheme);
  // These strings will be passed to the Initialize() method when an Application
  // is instantiated.
  // TODO(vtl): Maybe we should store/compare resolved URLs?
  void SetArgsForURL(const std::vector<std::string>& args, const GURL& url);
  // These options will be used in running any native application at |url|
  // (which shouldn't contain a query string). (|url| will be mapped and
  // resolved, and any application whose base resolved URL matches it will have
  // |options| applied.)
  // Note: Calling this for a URL will add (default) options for that URL if
  // necessary.
  // TODO(vtl): This may not do what's desired if the resolved URL results in an
  // HTTP redirect. Really, we want options to be identified with a particular
  // implementation, maybe via a signed manifest or something like that.
  NativeApplicationOptions* GetNativeApplicationOptionsForURL(const GURL& url);

  // Destroys all Shell-ends of connections established with Applications.
  // Applications connected by this ApplicationManager will observe pipe errors
  // and have a chance to shutdown.
  void TerminateShellConnections();

  // Removes a ShellImpl when it encounters an error.
  void OnShellImplError(ShellImpl* shell_impl);

 private:
  class ContentHandlerConnection;

  using URLToLoaderMap = std::map<GURL, scoped_ptr<ApplicationLoader>>;
  using SchemeToLoaderMap =
      std::map<std::string, scoped_ptr<ApplicationLoader>>;
  using IdentityToShellImplMap = std::map<Identity, scoped_ptr<ShellImpl>>;
  using IdentityToContentHandlerMap =
      std::map<Identity, scoped_ptr<ContentHandlerConnection>>;
  using URLToArgsMap = std::map<GURL, std::vector<std::string>>;
  using MimeTypeToURLMap = std::map<std::string, GURL>;
  using URLToNativeOptionsMap = std::map<GURL, NativeApplicationOptions>;

  void ConnectToApplicationWithParameters(
      const GURL& application_url,
      const GURL& requestor_url,
      mojo::InterfaceRequest<mojo::ServiceProvider> services,
      const base::Closure& on_application_end,
      const std::vector<std::string>& pre_redirect_parameters);

  bool ConnectToRunningApplication(
      const GURL& resolved_url,
      const GURL& requestor_url,
      mojo::InterfaceRequest<mojo::ServiceProvider>* services);

  bool ConnectToApplicationWithLoader(
      const GURL& resolved_url,
      const GURL& requestor_url,
      mojo::InterfaceRequest<mojo::ServiceProvider>* services,
      const base::Closure& on_application_end,
      const std::vector<std::string>& parameters,
      ApplicationLoader* loader);

  // Creates an Identity for the service identified by |resolved_url|.
  // If |new_process_per_connection| is true for the URL's options, then the
  // identity is unique. Otherwise, repeated invocations with the same
  // |resolved_url| will result in an equivalent Identity. If |strip_query| is
  // true, the query is stripped before creating the Identity.
  Identity MakeApplicationIdentity(const GURL& resolved_url,
                                   bool strip_query = true);

  mojo::InterfaceRequest<mojo::Application> RegisterShell(
      // The URL after resolution and redirects, including the querystring.
      const GURL& resolved_url,
      const GURL& requestor_url,
      mojo::InterfaceRequest<mojo::ServiceProvider> services,
      const base::Closure& on_application_end,
      const std::vector<std::string>& parameters);

  ShellImpl* GetShellImpl(const GURL& url);

  void ConnectToClient(ShellImpl* shell_impl,
                       const GURL& resolved_url,
                       const GURL& requestor_url,
                       mojo::InterfaceRequest<mojo::ServiceProvider> services);

  void HandleFetchCallback(
      const GURL& requestor_url,
      mojo::InterfaceRequest<mojo::ServiceProvider> services,
      const base::Closure& on_application_end,
      const std::vector<std::string>& parameters,
      scoped_ptr<Fetcher> fetcher);

  void RunNativeApplication(
      mojo::InterfaceRequest<mojo::Application> application_request,
      const NativeApplicationOptions& options,
      scoped_ptr<Fetcher> fetcher,
      const base::FilePath& file_path,
      bool path_exists);

  void LoadWithContentHandler(
      const GURL& content_handler_url,
      mojo::InterfaceRequest<mojo::Application> application_request,
      mojo::URLResponsePtr url_response);

  // Returns the appropriate loader for |url|, or null if there is no loader
  // configured for the URL.
  ApplicationLoader* GetLoaderForURL(const GURL& url);

  // Removes a mojo::ContentHandler when it encounters an error.
  void OnContentHandlerError(ContentHandlerConnection* content_handler);

  // Returns the arguments for the given url.
  std::vector<std::string> GetArgsForURL(const GURL& url);

  void CleanupRunner(NativeRunner* runner);

  const Options options_;
  Delegate* const delegate_;
  // Loader management.
  // Loaders are chosen in the order they are listed here.
  URLToLoaderMap url_to_loader_;
  SchemeToLoaderMap scheme_to_loader_;
  scoped_ptr<ApplicationLoader> default_loader_;
  scoped_ptr<NativeRunnerFactory> native_runner_factory_;

  IdentityToShellImplMap identity_to_shell_impl_;
  IdentityToContentHandlerMap identity_to_content_handler_;
  URLToArgsMap url_to_args_;
  // Note: The keys are URLs after mapping and resolving.
  URLToNativeOptionsMap url_to_native_options_;

  base::SequencedWorkerPool* blocking_pool_;
  mojo::URLResponseDiskCachePtr url_response_disk_cache_;
  mojo::NetworkServicePtr network_service_;
  mojo::NetworkServicePtr authenticating_network_service_;
  MimeTypeToURLMap mime_type_to_url_;
  ScopedVector<NativeRunner> native_runners_;
  bool initialized_authentication_interceptor_;
  base::WeakPtrFactory<ApplicationManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ApplicationManager);
};

}  // namespace shell

#endif  // SHELL_APPLICATION_MANAGER_APPLICATION_MANAGER_H_
