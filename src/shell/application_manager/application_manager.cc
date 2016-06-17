// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/application_manager/application_manager.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/trace_event/trace_event.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/services/authenticating_url_loader_interceptor/interfaces/authenticating_url_loader_interceptor_meta_factory.mojom.h"
#include "mojo/services/authentication/interfaces/authentication.mojom.h"
#include "mojo/services/content_handler/interfaces/content_handler.mojom.h"
#include "shell/application_manager/fetcher.h"
#include "shell/application_manager/local_fetcher.h"
#include "shell/application_manager/network_fetcher.h"
#include "shell/application_manager/query_util.h"
#include "shell/application_manager/shell_impl.h"

using mojo::Application;
using mojo::ApplicationPtr;
using mojo::InterfaceRequest;
using mojo::ServiceProvider;
using mojo::ServiceProviderPtr;

namespace shell {

namespace {

// Create identity that depends on the query.
const bool kDoNotStripQuery = false;

// Used by TestAPI.
bool has_created_instance = false;

std::vector<std::string> Concatenate(const std::vector<std::string>& v1,
                                     const std::vector<std::string>& v2) {
  if (!v1.size())
    return v2;
  if (!v2.size())
    return v1;
  std::vector<std::string> result(v1);
  result.insert(result.end(), v1.begin(), v1.end());
  return result;
}

void AppendArgsForURL(const GURL& url,
                      const std::vector<std::string>& args,
                      std::vector<std::string>* target) {
  if (target->empty())
    target->push_back(url.spec());
  target->insert(target->end(), args.begin(), args.end());
}

}  // namespace

class ApplicationManager::ContentHandlerConnection {
 public:
  ContentHandlerConnection(ApplicationManager* manager, Identity identity)
      : manager_(manager), identity_(identity) {
    ServiceProviderPtr services;
    manager->ConnectToApplication(identity_.url, GURL(),
                                  mojo::GetProxy(&services), base::Closure());
    mojo::MessagePipe pipe;
    content_handler_.Bind(
        mojo::InterfaceHandle<mojo::ContentHandler>(pipe.handle0.Pass(), 0u));
    services->ConnectToService(mojo::ContentHandler::Name_,
                               pipe.handle1.Pass());
    content_handler_.set_connection_error_handler(
        [this]() { manager_->OnContentHandlerError(this); });
  }
  ~ContentHandlerConnection() {}

  mojo::ContentHandler* content_handler() { return content_handler_.get(); }

  const GURL& content_handler_url() const { return identity_.url; }

  const Identity& identity() const { return identity_; }

 private:
  ApplicationManager* manager_;
  const Identity identity_;
  mojo::ContentHandlerPtr content_handler_;

  DISALLOW_COPY_AND_ASSIGN(ContentHandlerConnection);
};

// static
ApplicationManager::TestAPI::TestAPI(ApplicationManager* manager)
    : manager_(manager) {
}

ApplicationManager::TestAPI::~TestAPI() {
}

bool ApplicationManager::TestAPI::HasCreatedInstance() {
  return has_created_instance;
}

bool ApplicationManager::TestAPI::HasFactoryForURL(const GURL& url) const {
  DCHECK(!url.has_query());
  return manager_->identity_to_shell_impl_.find(Identity(url)) !=
         manager_->identity_to_shell_impl_.end();
}

ApplicationManager::ApplicationManager(const Options& options,
                                       Delegate* delegate)
    : options_(options),
      delegate_(delegate),
      blocking_pool_(nullptr),
      initialized_authentication_interceptor_(false),
      weak_ptr_factory_(this) {
}

ApplicationManager::~ApplicationManager() {
}

void ApplicationManager::TerminateShellConnections() {
  identity_to_shell_impl_.clear();
}

void ApplicationManager::ConnectToApplication(
    const GURL& requested_url,
    const GURL& requestor_url,
    InterfaceRequest<ServiceProvider> services,
    const base::Closure& on_application_end) {
  ConnectToApplicationWithParameters(requested_url, requestor_url,
                                     services.Pass(), on_application_end,
                                     std::vector<std::string>());
}

void ApplicationManager::ConnectToApplicationWithParameters(
    const GURL& requested_url,
    const GURL& requestor_url,
    InterfaceRequest<ServiceProvider> services,
    const base::Closure& on_application_end,
    const std::vector<std::string>& pre_redirect_parameters) {
  TRACE_EVENT_INSTANT2(
      "mojo_shell", "ApplicationManager::ConnectToApplicationWithParameters",
      TRACE_EVENT_SCOPE_THREAD, "requested_url", requested_url.spec(),
      "requestor_url", requestor_url.spec());
  DCHECK(requested_url.is_valid());

  // We check both the mapped and resolved urls for existing shell_impls because
  // external applications can be registered for the unresolved mojo:foo urls.

  GURL mapped_url = delegate_->ResolveMappings(requested_url);
  if (ConnectToRunningApplication(mapped_url, requestor_url, &services))
    return;

  GURL resolved_url = delegate_->ResolveMojoURL(mapped_url);
  if (ConnectToRunningApplication(resolved_url, requestor_url, &services))
    return;

  // The application is not running, let's compute the parameters.
  std::vector<std::string> parameters =
      Concatenate(pre_redirect_parameters, GetArgsForURL(resolved_url));

  if (ConnectToApplicationWithLoader(mapped_url, requestor_url, &services,
                                     on_application_end, parameters,
                                     GetLoaderForURL(mapped_url)))
    return;

  if (ConnectToApplicationWithLoader(resolved_url, requestor_url, &services,
                                     on_application_end, parameters,
                                     GetLoaderForURL(resolved_url)))
    return;

  if (ConnectToApplicationWithLoader(resolved_url, requestor_url, &services,
                                     on_application_end, parameters,
                                     default_loader_.get()))
    return;

  auto callback =
      base::Bind(&ApplicationManager::HandleFetchCallback,
                 weak_ptr_factory_.GetWeakPtr(), requestor_url,
                 base::Passed(services.Pass()), on_application_end, parameters);

  if (resolved_url.SchemeIsFile()) {
    new LocalFetcher(resolved_url, GetBaseURLAndQuery(resolved_url, nullptr),
                     callback);
    return;
  }

  if (!url_response_disk_cache_) {
    ConnectToService(GURL("mojo:url_response_disk_cache"),
                     &url_response_disk_cache_);
    ConnectToService(GURL("mojo:network_service"), &network_service_);
    ConnectToService(GURL("mojo:network_service"),
                     &authenticating_network_service_);
  }

  mojo::NetworkService* network_service = authenticating_network_service_.get();

  // NOTE: Attempting to initialize the apps used authentication for while
  // connecting to those apps would result in a recursive loop, so it has to be
  // explicitly avoided here. What this means in practice is that these apps
  // cannot themselves require authentication to obtain.
  if (base::EndsWith(resolved_url.path(), "/authentication.mojo",
                     base::CompareCase::SENSITIVE) ||
      base::EndsWith(resolved_url.path(),
                     "/authenticating_url_loader_interceptor.mojo",
                     base::CompareCase::SENSITIVE)) {
    network_service = network_service_.get();
  } else if (!initialized_authentication_interceptor_) {
#ifndef NO_AUTHENTICATION
    // TODO(toshik): FNL hasn't supported authentication, yet
    authentication::AuthenticationServicePtr authentication_service;
    ConnectToService(GURL("mojo:authentication"), &authentication_service);
    mojo::AuthenticatingURLLoaderInterceptorMetaFactoryPtr
        interceptor_meta_factory;
    ConnectToService(GURL("mojo:authenticating_url_loader_interceptor"),
                     &interceptor_meta_factory);
    mojo::URLLoaderInterceptorFactoryPtr interceptor_factory;
    interceptor_meta_factory->CreateURLLoaderInterceptorFactory(
        GetProxy(&interceptor_factory), authentication_service.Pass());
    authenticating_network_service_->RegisterURLLoaderInterceptor(
        interceptor_factory.Pass());
#endif
    initialized_authentication_interceptor_ = true;
  }

  new NetworkFetcher(options_.disable_cache, options_.force_offline_by_default,
                     resolved_url, url_response_disk_cache_.get(),
                     network_service, callback);
}

bool ApplicationManager::ConnectToRunningApplication(
    const GURL& resolved_url,
    const GURL& requestor_url,
    InterfaceRequest<ServiceProvider>* services) {
  GURL application_url = GetBaseURLAndQuery(resolved_url, nullptr);
  ShellImpl* shell_impl = GetShellImpl(application_url);
  if (!shell_impl)
    return false;

  DCHECK(!GetNativeApplicationOptionsForURL(application_url)
              ->new_process_per_connection);

  ConnectToClient(shell_impl, resolved_url, requestor_url, services->Pass());
  return true;
}

bool ApplicationManager::ConnectToApplicationWithLoader(
    const GURL& resolved_url,
    const GURL& requestor_url,
    InterfaceRequest<ServiceProvider>* services,
    const base::Closure& on_application_end,
    const std::vector<std::string>& parameters,
    ApplicationLoader* loader) {
  if (!loader)
    return false;

  loader->Load(resolved_url,
               RegisterShell(resolved_url, requestor_url, services->Pass(),
                             on_application_end, parameters));
  return true;
}

Identity ApplicationManager::MakeApplicationIdentity(const GURL& resolved_url,
                                                     bool strip_query) {
  static uint64_t unique_id_number = 1;
  GURL stripped_url = GetBaseURLAndQuery(resolved_url, nullptr);
  GURL url = strip_query ? stripped_url : resolved_url;
  bool new_process_per_connection =
      GetNativeApplicationOptionsForURL(stripped_url)
          ->new_process_per_connection;
  return new_process_per_connection
             ? Identity(url, base::Uint64ToString(unique_id_number++))
             : Identity(url);
}

InterfaceRequest<Application> ApplicationManager::RegisterShell(
    const GURL& resolved_url,
    const GURL& requestor_url,
    InterfaceRequest<ServiceProvider> services,
    const base::Closure& on_application_end,
    const std::vector<std::string>& parameters) {
  Identity app_identity = MakeApplicationIdentity(resolved_url);

  mojo::ApplicationPtr application;
  InterfaceRequest<Application> application_request =
      mojo::GetProxy(&application);
  ShellImpl* shell =
      new ShellImpl(application.Pass(), this, app_identity, on_application_end);
  identity_to_shell_impl_[app_identity] = make_scoped_ptr(shell);
  shell->InitializeApplication(mojo::Array<mojo::String>::From(parameters));
  ConnectToClient(shell, resolved_url, requestor_url, services.Pass());
  return application_request;
}

// Note: If a service was created with a unique ID, intending to be unique
// (such that multiple requests for a service result in unique processes), then
// 'GetShellImpl' should return nullptr.
ShellImpl* ApplicationManager::GetShellImpl(const GURL& url) {
  DCHECK(!url.has_query());
  const auto& shell_it = identity_to_shell_impl_.find(Identity(url));
  if (shell_it != identity_to_shell_impl_.end())
    return shell_it->second.get();
  return nullptr;
}

void ApplicationManager::ConnectToClient(
    ShellImpl* shell_impl,
    const GURL& resolved_url,
    const GURL& requestor_url,
    InterfaceRequest<ServiceProvider> services) {
  shell_impl->ConnectToClient(resolved_url, requestor_url, services.Pass());
}

void ApplicationManager::HandleFetchCallback(
    const GURL& requestor_url,
    InterfaceRequest<ServiceProvider> services,
    const base::Closure& on_application_end,
    const std::vector<std::string>& parameters,
    scoped_ptr<Fetcher> fetcher) {
  if (!fetcher) {
    // Network error. Drop |application_request| to tell requestor.
    return;
  }

  GURL redirect_url = fetcher->GetRedirectURL();
  if (!redirect_url.is_empty()) {
    // And around we go again... Whee!
    ConnectToApplicationWithParameters(redirect_url, requestor_url,
                                       services.Pass(), on_application_end,
                                       parameters);
    return;
  }

  // We already checked if the application was running before we fetched it, but
  // it might have started while the fetch was outstanding. We don't want to
  // have two copies of the app running, so check again.
  //
  // Also, it's possible the original URL was redirected to an app that is
  // already running.
  if (ConnectToRunningApplication(fetcher->GetURL(), requestor_url, &services))
    return;

  InterfaceRequest<Application> request(
      RegisterShell(fetcher->GetURL(), requestor_url, services.Pass(),
                    on_application_end, parameters));

  // If the response begins with a #!mojo <content-handler-url>, use it.
  GURL content_handler_url;
  std::string shebang;
  if (fetcher->PeekContentHandler(&shebang, &content_handler_url)) {
    LoadWithContentHandler(
        content_handler_url, request.Pass(),
        fetcher->AsURLResponse(blocking_pool_,
                               static_cast<int>(shebang.size())));
    return;
  }

  auto it = mime_type_to_url_.find(fetcher->MimeType());
  if (it != mime_type_to_url_.end()) {
    LoadWithContentHandler(it->second, request.Pass(),
                           fetcher->AsURLResponse(blocking_pool_, 0));
    return;
  }

  // TODO(aa): Sanity check that the thing we got looks vaguely like a mojo
  // application. That could either mean looking for the platform-specific dll
  // header, or looking for some specific mojo signature prepended to the
  // library.
  // TODO(vtl): (Maybe this should be done by the factory/runner?)

  GURL base_resolved_url = GetBaseURLAndQuery(fetcher->GetURL(), nullptr);
  NativeApplicationOptions options;
  if (url_to_native_options_.find(base_resolved_url) !=
      url_to_native_options_.end()) {
    DVLOG(2) << "Applying stored native options to resolved URL "
             << fetcher->GetURL();
    options = url_to_native_options_[base_resolved_url];
  }

  TRACE_EVENT_ASYNC_BEGIN1("mojo_shell", "ApplicationManager::RetrievePath",
                           fetcher.get(), "url", fetcher->GetURL().spec());
  fetcher->AsPath(
      blocking_pool_,
      base::Bind(&ApplicationManager::RunNativeApplication,
                 weak_ptr_factory_.GetWeakPtr(), base::Passed(request.Pass()),
                 options, base::Passed(fetcher.Pass())));
}

void ApplicationManager::RunNativeApplication(
    InterfaceRequest<Application> application_request,
    const NativeApplicationOptions& options,
    scoped_ptr<Fetcher> fetcher,
    const base::FilePath& path,
    bool path_exists) {
  TRACE_EVENT_ASYNC_END0("mojo_shell", "ApplicationManager::RetrievePath",
                         fetcher.get());
  // We only passed fetcher to keep it alive. Done with it now.
  fetcher.reset();

  DCHECK(application_request.is_pending());

  if (!path_exists) {
    LOG(ERROR) << "Library not started because library path '" << path.value()
               << "' does not exist.";
    return;
  }

  TRACE_EVENT1("mojo_shell", "ApplicationManager::RunNativeApplication", "path",
               path.AsUTF8Unsafe());
  NativeRunner* runner = native_runner_factory_->Create(options).release();
  native_runners_.push_back(runner);
  runner->Start(path, application_request.Pass(),
                base::Bind(&ApplicationManager::CleanupRunner,
                           weak_ptr_factory_.GetWeakPtr(), runner));
}

void ApplicationManager::RegisterContentHandler(
    const std::string& mime_type,
    const GURL& content_handler_url) {
  DCHECK(content_handler_url.is_valid())
      << "Content handler URL is invalid for mime type " << mime_type;
  mime_type_to_url_[mime_type] = content_handler_url;
}

void ApplicationManager::LoadWithContentHandler(
    const GURL& content_handler_url,
    InterfaceRequest<Application> application_request,
    mojo::URLResponsePtr url_response) {
  ContentHandlerConnection* connection = nullptr;
  // If two content handler urls differ by query parameter, we want to create a
  // separate connection for each.
  Identity content_handler_id =
      MakeApplicationIdentity(content_handler_url, kDoNotStripQuery);
  auto it = identity_to_content_handler_.find(content_handler_id);
  if (it != identity_to_content_handler_.end()) {
    connection = it->second.get();
  } else {
    connection = new ContentHandlerConnection(this, content_handler_id);
    identity_to_content_handler_[content_handler_id] =
        make_scoped_ptr(connection);
  }

  connection->content_handler()->StartApplication(application_request.Pass(),
                                                  url_response.Pass());
}

void ApplicationManager::SetLoaderForURL(scoped_ptr<ApplicationLoader> loader,
                                         const GURL& url) {
  url_to_loader_[url] = loader.Pass();
}

void ApplicationManager::SetLoaderForScheme(
    scoped_ptr<ApplicationLoader> loader,
    const std::string& scheme) {
  scheme_to_loader_[scheme] = loader.Pass();
}

void ApplicationManager::SetArgsForURL(const std::vector<std::string>& args,
                                       const GURL& url) {
  GURL base_url = GetBaseURLAndQuery(url, nullptr);
  AppendArgsForURL(base_url, args, &url_to_args_[base_url]);
  GURL mapped_url = delegate_->ResolveMappings(base_url);
  DCHECK(!mapped_url.has_query());
  if (mapped_url != base_url)
    AppendArgsForURL(mapped_url, args, &url_to_args_[mapped_url]);
  GURL resolved_url = delegate_->ResolveMojoURL(mapped_url);
  DCHECK(!resolved_url.has_query());
  if (resolved_url != mapped_url)
    AppendArgsForURL(resolved_url, args, &url_to_args_[resolved_url]);
}

NativeApplicationOptions* ApplicationManager::GetNativeApplicationOptionsForURL(
    const GURL& url) {
  DCHECK(!url.has_query());  // Precondition.
  // Apply mappings and resolution to get the resolved URL.
  GURL resolved_url =
      delegate_->ResolveMojoURL(delegate_->ResolveMappings(url));
  // TODO(vtl): We should probably also remove/disregard the query string (and
  // maybe canonicalize in other ways).
  DCHECK(!resolved_url.has_query());  // Still shouldn't have query.
  return &url_to_native_options_[resolved_url];
}

ApplicationLoader* ApplicationManager::GetLoaderForURL(const GURL& url) {
  auto url_it = url_to_loader_.find(GetBaseURLAndQuery(url, nullptr));
  if (url_it != url_to_loader_.end())
    return url_it->second.get();
  auto scheme_it = scheme_to_loader_.find(url.scheme());
  if (scheme_it != scheme_to_loader_.end())
    return scheme_it->second.get();
  return nullptr;
}

void ApplicationManager::OnShellImplError(ShellImpl* shell_impl) {
  // Called from ~ShellImpl, so we do not need to call Destroy here.
  const Identity identity = shell_impl->identity();
  base::Closure on_application_end = shell_impl->on_application_end();
  // Remove the shell.
  auto it = identity_to_shell_impl_.find(identity);
  DCHECK(it != identity_to_shell_impl_.end());
  identity_to_shell_impl_.erase(it);
  if (!on_application_end.is_null())
    on_application_end.Run();
}

void ApplicationManager::OnContentHandlerError(
    ContentHandlerConnection* content_handler) {
  // Remove the mapping to the content handler.
  auto it = identity_to_content_handler_.find(content_handler->identity());
  DCHECK(it != identity_to_content_handler_.end());
  identity_to_content_handler_.erase(it);
}

mojo::ScopedMessagePipeHandle ApplicationManager::ConnectToServiceByName(
    const GURL& application_url,
    const std::string& interface_name) {
  ServiceProviderPtr services;
  ConnectToApplication(application_url, GURL(), mojo::GetProxy(&services),
                       base::Closure());
  mojo::MessagePipe pipe;
  services->ConnectToService(interface_name, pipe.handle1.Pass());
  return pipe.handle0.Pass();
}

std::vector<std::string> ApplicationManager::GetArgsForURL(const GURL& url) {
  GURL base_url = GetBaseURLAndQuery(url, nullptr);
  const auto& args_it = url_to_args_.find(base_url);
  if (args_it != url_to_args_.end())
    return args_it->second;
  return std::vector<std::string>();
}

void ApplicationManager::CleanupRunner(NativeRunner* runner) {
  native_runners_.erase(
      std::find(native_runners_.begin(), native_runners_.end(), runner));
}

}  // namespace shell
