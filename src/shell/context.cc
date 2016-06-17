// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/context.h"

#include <vector>

#include "base/base_switches.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "mojo/common/trace_provider_impl.h"
#include "mojo/common/tracing_impl.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/multiprocess_embedder.h"
#include "mojo/edk/embedder/simple_platform_support.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/services/tracing/interfaces/trace_provider_registry.mojom.h"
#include "mojo/services/tracing/interfaces/tracing.mojom.h"
#include "shell/application_manager/application_loader.h"
#include "shell/application_manager/application_manager.h"
#include "shell/application_manager/native_application_options.h"
#include "shell/background_application_loader.h"
#include "shell/command_line_util.h"
#include "shell/filename_util.h"
#include "shell/in_process_native_runner.h"
#include "shell/out_of_process_native_runner.h"
#include "shell/switches.h"
#include "shell/tracer.h"
#include "url/gurl.h"

#if !defined(OS_MACOSX)
#include "shell/url_response_disk_cache_loader.h"
#endif

using mojo::ServiceProvider;
using mojo::ServiceProviderPtr;

namespace shell {
namespace {

ApplicationManager::Options MakeApplicationManagerOptions() {
  ApplicationManager::Options options;
  options.disable_cache = base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kDisableCache);
  options.force_offline_by_default =
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kForceOfflineByDefault);
  return options;
}

bool ConfigureURLMappings(const base::CommandLine& command_line,
                          Context* context) {
  URLResolver* resolver = context->url_resolver();

  // Configure the resolution of unknown mojo: URLs.
  GURL base_url;
  if (command_line.HasSwitch(switches::kOrigin))
    base_url = GURL(command_line.GetSwitchValueASCII(switches::kOrigin));
  else
    // Use the shell's file root if the base was not specified.
    base_url = context->ResolveShellFileURL("");

  if (!base_url.is_valid())
    return false;

  resolver->SetMojoBaseURL(base_url);

  // The network service must be loaded from the filesystem.
  // This mapping is done before the command line URL mapping are processed, so
  // that it can be overridden.
  resolver->AddURLMapping(
      GURL("mojo:network_service"),
      context->ResolveShellFileURL("file:network_service.mojo"));

  // Command line URL mapping.
  std::vector<URLResolver::OriginMapping> origin_mappings =
      URLResolver::GetOriginMappings(command_line.argv());
  for (const auto& origin_mapping : origin_mappings)
    resolver->AddOriginMapping(GURL(origin_mapping.origin),
                               GURL(origin_mapping.base_url));

  if (command_line.HasSwitch(switches::kURLMappings)) {
    const std::string mappings =
        command_line.GetSwitchValueASCII(switches::kURLMappings);

    base::StringPairs pairs;
    if (!base::SplitStringIntoKeyValuePairs(mappings, '=', ',', &pairs))
      return false;
    using StringPair = std::pair<std::string, std::string>;
    for (const StringPair& pair : pairs) {
      const GURL from(pair.first);
      const GURL to = context->ResolveCommandLineURL(pair.second);
      if (!from.is_valid() || !to.is_valid())
        return false;
      resolver->AddURLMapping(from, to);
    }
  }
  return true;
}

void InitContentHandlers(ApplicationManager* manager,
                         const base::CommandLine& command_line) {
  // Default content handlers.
  manager->RegisterContentHandler("application/pdf", GURL("mojo:pdf_viewer"));
  manager->RegisterContentHandler("image/png", GURL("mojo:png_viewer"));
  manager->RegisterContentHandler("text/html", GURL("mojo:html_viewer"));

  // Command-line-specified content handlers.
  std::string handlers_spec =
      command_line.GetSwitchValueASCII(switches::kContentHandlers);
  if (handlers_spec.empty())
    return;

#if defined(OS_ANDROID)
  // TODO(eseidel): On Android we pass command line arguments is via the
  // 'parameters' key on the intent, which we specify during 'am shell start'
  // via --esa, however that expects comma-separated values and says:
  //   am shell --help:
  //     [--esa <EXTRA_KEY> <EXTRA_STRING_VALUE>[,<EXTRA_STRING_VALUE...]]
  //     (to embed a comma into a string escape it using "\,")
  // Whatever takes 'parameters' and constructs a CommandLine is failing to
  // un-escape the commas, we need to move this fix to that file.
  base::ReplaceSubstringsAfterOffset(&handlers_spec, 0, "\\,", ",");
#endif

  std::vector<std::string> parts = base::SplitString(
      handlers_spec, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  if (parts.size() % 2 != 0) {
    LOG(ERROR) << "Invalid value for switch " << switches::kContentHandlers
               << ": must be a comma-separated list of mimetype/url pairs."
               << " Value was: " << handlers_spec;
    return;
  }

  for (size_t i = 0; i < parts.size(); i += 2) {
    GURL url(parts[i + 1]);
    if (!url.is_valid()) {
      LOG(ERROR) << "Invalid value for switch " << switches::kContentHandlers
                 << ": '" << parts[i + 1] << "' is not a valid URL.";
      return;
    }
    // TODO(eseidel): We should also validate that the mimetype is valid
    // net/base/mime_util.h could do this, but we don't want to depend on net.
    manager->RegisterContentHandler(parts[i], url);
  }
}

void InitNativeOptions(ApplicationManager* manager,
                       const base::CommandLine& command_line) {
  std::vector<std::string> force_in_process_url_list = base::SplitString(
      command_line.GetSwitchValueASCII(switches::kForceInProcess), ",",
      base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  for (const auto& force_in_process_url : force_in_process_url_list) {
    GURL url(force_in_process_url);
    if (!url.is_valid()) {
      LOG(ERROR) << "Invalid value for switch " << switches::kForceInProcess
                 << ": '" << force_in_process_url << "'is not a valid URL.";
      return;
    }

    manager->GetNativeApplicationOptionsForURL(url)->force_in_process = true;
  }

  // TODO(vtl): This is a total hack. We should have a systematic way of
  // configuring options for native apps.
  manager->GetNativeApplicationOptionsForURL(GURL("mojo:native_support"))
      ->allow_new_privs = true;
  // All NaCl operations require a unique process
  manager->GetNativeApplicationOptionsForURL(GURL("mojo:pnacl_compile"))
      ->new_process_per_connection = true;
  manager->GetNativeApplicationOptionsForURL(GURL("mojo:pnacl_link"))
      ->new_process_per_connection = true;
  manager->GetNativeApplicationOptionsForURL(
             GURL("mojo:content_handler_nonsfi_pexe"))
      ->new_process_per_connection = true;
  manager->GetNativeApplicationOptionsForURL(
             GURL("mojo:content_handler_nonsfi_nexe"))
      ->new_process_per_connection = true;
}

}  // namespace

Context::Context(Tracer* tracer)
    : tracer_(tracer),
      application_manager_(MakeApplicationManagerOptions(), this) {
  DCHECK(!base::MessageLoop::current());

  // By default assume that the local apps reside alongside the shell.
  // TODO(ncbray): really, this should be passed in rather than defaulting.
  // This default makes sense for desktop but not Android.
  base::FilePath shell_dir;
  PathService::Get(base::DIR_MODULE, &shell_dir);
  SetShellFileRoot(shell_dir);

  base::FilePath cwd;
  PathService::Get(base::DIR_CURRENT, &cwd);
  SetCommandLineCWD(cwd);
}

Context::~Context() {
  DCHECK(!base::MessageLoop::current());
}

// static
void Context::EnsureEmbedderIsInitialized() {
  static bool initialized = ([]() {
    mojo::embedder::Init(mojo::embedder::CreateSimplePlatformSupport());
    return true;
  })();
  ALLOW_UNUSED_LOCAL(initialized);
}

void Context::SetShellFileRoot(const base::FilePath& path) {
  shell_file_root_ = AddTrailingSlashIfNeeded(FilePathToFileURL(path));
}

GURL Context::ResolveShellFileURL(const std::string& path) {
  return shell_file_root_.Resolve(path);
}

void Context::SetCommandLineCWD(const base::FilePath& path) {
  command_line_cwd_ = AddTrailingSlashIfNeeded(FilePathToFileURL(path));
}

GURL Context::ResolveCommandLineURL(const std::string& path) {
  return command_line_cwd_.Resolve(path);
}

bool Context::Init() {
  base::FilePath shell_path = base::MakeAbsoluteFilePath(
      base::CommandLine::ForCurrentProcess()->GetProgram());
  base::FilePath shell_child_path =
      shell_path.DirName().AppendASCII("mojo_shell_child");
  return InitWithPaths(shell_child_path, nullptr);
}

bool Context::InitWithPaths(
    const base::FilePath& shell_child_path,
    mojo::URLResponseDiskCacheDelegate* url_response_disk_cache_delegate) {
  TRACE_EVENT0("mojo_shell", "Context::InitWithPaths");
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();

  if (command_line.HasSwitch(switches::kWaitForDebugger))
    base::debug::WaitForDebugger(60, true);

  mojo_shell_child_path_ = shell_child_path;

  task_runners_.reset(
      new TaskRunners(base::MessageLoop::current()->task_runner()));

#if !defined(OS_MACOSX)
  application_manager()->SetLoaderForURL(
      make_scoped_ptr(new BackgroundApplicationLoader(
          make_scoped_ptr(
              new URLResponseDiskCacheLoader(task_runners_->blocking_pool(),
                                             url_response_disk_cache_delegate)),
          "url_response_disk_cache", base::MessageLoop::TYPE_DEFAULT)),
      GURL("mojo:url_response_disk_cache"));
#endif

  EnsureEmbedderIsInitialized();

  // TODO(vtl): Probably these failures should be checked before |Init()|, and
  // this function simply shouldn't fail.
  if (!shell_file_root_.is_valid())
    return false;
  if (!ConfigureURLMappings(command_line, this))
    return false;

  mojo::embedder::InitIPCSupport(mojo::embedder::ProcessType::MASTER,
                                 task_runners_->shell_runner().Clone(), this,
                                 task_runners_->io_runner().Clone(),
                                 task_runners_->io_watcher(),
                                 mojo::platform::ScopedPlatformHandle());

  scoped_ptr<NativeRunnerFactory> runner_factory;
  if (command_line.HasSwitch(switches::kEnableMultiprocess))
    runner_factory.reset(new OutOfProcessNativeRunnerFactory(this));
  else
    runner_factory.reset(new InProcessNativeRunnerFactory(this));
  application_manager_.set_blocking_pool(task_runners_->blocking_pool());
  application_manager_.set_native_runner_factory(runner_factory.Pass());

  InitContentHandlers(&application_manager_, command_line);
  InitNativeOptions(&application_manager_, command_line);

  // The mojo_shell --args-for command-line switch is handled specially because
  // it can appear more than once. The base::CommandLine class collapses
  // multiple occurrences of the same switch.
  base::CommandLine* current = base::CommandLine::ForCurrentProcess();
  for (size_t i = 1; i < current->argv().size(); ++i)
    ApplyApplicationArgs(this, current->argv()[i]);

  ServiceProviderPtr tracing_services;
  application_manager_.ConnectToApplication(GURL("mojo:tracing"), GURL(""),
                                            GetProxy(&tracing_services),
                                            base::Closure());
  if (tracer_) {
    tracing::TraceProviderRegistryPtr registry;
    mojo::ConnectToService(tracing_services.get(), GetProxy(&registry));

    mojo::InterfaceHandle<tracing::TraceProvider> provider;
    tracer_->ConnectToProvider(GetProxy(&provider));
    registry->RegisterTraceProvider(provider.Pass());
  }

  if (command_line.HasSwitch(switches::kTraceStartup)) {
    DCHECK(tracer_);
    tracing::TraceCollectorPtr coordinator;
    mojo::ConnectToService(tracing_services.get(), GetProxy(&coordinator));
    tracer_->StartCollectingFromTracingService(coordinator.Pass());
  }

  return true;
}

void Context::Shutdown() {
  TRACE_EVENT0("mojo_shell", "Context::Shutdown");
  DCHECK(task_runners_->shell_runner()->RunsTasksOnCurrentThread());
  mojo::embedder::ShutdownIPCSupport();
  // We'll quit when we get OnShutdownComplete().
  base::MessageLoop::current()->Run();
}

GURL Context::ResolveMappings(const GURL& url) {
  return url_resolver_.ApplyMappings(url);
}

GURL Context::ResolveMojoURL(const GURL& url) {
  return url_resolver_.ResolveMojoURL(url);
}

void Context::OnShutdownComplete() {
  DCHECK(task_runners_->shell_runner()->RunsTasksOnCurrentThread());
  base::MessageLoop::current()->Quit();
}

void Context::OnSlaveDisconnect(mojo::embedder::SlaveInfo slave_info) {
  // TODO(vtl): Do something, once we actually have |slave_info|.
}

void Context::Run(const GURL& url) {
  ServiceProviderPtr services;

  app_urls_.insert(url);
  application_manager_.ConnectToApplication(
      url, GURL(), mojo::GetProxy(&services),
      base::Bind(&Context::OnApplicationEnd, base::Unretained(this), url));
}

void Context::OnApplicationEnd(const GURL& url) {
  if (app_urls_.find(url) != app_urls_.end()) {
    app_urls_.erase(url);
    if (app_urls_.empty() && base::MessageLoop::current()->is_running()) {
      DCHECK(task_runners_->shell_runner()->RunsTasksOnCurrentThread());
      base::MessageLoop::current()->Quit();
    }
  }
}

}  // namespace shell
