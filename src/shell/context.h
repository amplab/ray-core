// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_CONTEXT_H_
#define SHELL_CONTEXT_H_

#include <string>

#include "base/macros.h"
#include "mojo/edk/embedder/master_process_delegate.h"
#include "shell/application_manager/application_manager.h"
#include "shell/task_runners.h"
#include "shell/url_resolver.h"

namespace mojo {
class URLResponseDiskCacheDelegate;
}

namespace shell {
class Tracer;

// The "global" context for the shell's main process.
class Context : public ApplicationManager::Delegate,
                public mojo::embedder::MasterProcessDelegate {
 public:
  explicit Context(Tracer* tracer = nullptr);
  ~Context() override;

  static void EnsureEmbedderIsInitialized();

  // Point to the directory containing installed services, such as the network
  // service. By default this directory is used as the base URL for resolving
  // unknown mojo: URLs. The network service will be loaded from this directory,
  // even when the base URL for unknown mojo: URLs is overridden.
  void SetShellFileRoot(const base::FilePath& path);

  // Resolve an URL relative to the shell file root. This is a nop for
  // everything but relative file URLs or URLs without a scheme.
  GURL ResolveShellFileURL(const std::string& path);

  // Override the CWD, which is used for resolving file URLs passed in from the
  // command line.
  void SetCommandLineCWD(const base::FilePath& path);

  // Resolve an URL relative to the CWD mojo_shell was invoked from. This is a
  // nop for everything but relative file URLs or URLs without a scheme.
  GURL ResolveCommandLineURL(const std::string& path);

  // This must be called with a message loop set up for the current thread,
  // which must remain alive until after Shutdown() is called. Returns true on
  // success.
  bool Init();

  // Like Init(), but specifies |mojo_shell_child_path()| explicitly. Also
  // allows to set a delegate for the application offline cache.
  bool InitWithPaths(
      const base::FilePath& shell_child_path,
      mojo::URLResponseDiskCacheDelegate* url_response_disk_cache_delegate);

  // If Init() was called and succeeded, this must be called before destruction.
  void Shutdown();

  void Run(const GURL& url);

  ApplicationManager* application_manager() { return &application_manager_; }
  URLResolver* url_resolver() { return &url_resolver_; }

  // Available after Init():
  // This is an absolute path.
  const base::FilePath& mojo_shell_child_path() const {
    return mojo_shell_child_path_;
  }
  TaskRunners* task_runners() { return task_runners_.get(); }

 private:
  class NativeViewportApplicationLoader;

  // ApplicationManager::Delegate overrides.
  GURL ResolveMappings(const GURL& url) override;
  GURL ResolveMojoURL(const GURL& url) override;

  // MasterProcessDelegate implementation.
  void OnShutdownComplete() override;
  void OnSlaveDisconnect(mojo::embedder::SlaveInfo slave_info) override;

  void OnApplicationEnd(const GURL& url);

  Tracer* const tracer_;
  ApplicationManager application_manager_;
  URLResolver url_resolver_;

  base::FilePath mojo_shell_child_path_;
  scoped_ptr<TaskRunners> task_runners_;

  std::set<GURL> app_urls_;
  GURL shell_file_root_;
  GURL command_line_cwd_;

  DISALLOW_COPY_AND_ASSIGN(Context);
};

}  // namespace shell

#endif  // SHELL_CONTEXT_H_
