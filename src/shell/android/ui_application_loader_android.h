// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_ANDROID_UI_APPLICATION_LOADER_ANDROID_H_
#define SHELL_ANDROID_UI_APPLICATION_LOADER_ANDROID_H_

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "shell/application_manager/application_loader.h"

namespace base {
class MessageLoop;
}

namespace shell {

class ApplicationManager;

// ApplicationLoader implementation that creates a background thread and issues
// load
// requests there.
class UIApplicationLoader : public ApplicationLoader {
 public:
  UIApplicationLoader(scoped_ptr<ApplicationLoader> real_loader,
                      base::MessageLoop* ui_message_loop);
  ~UIApplicationLoader() override;

  // ApplicationLoader overrides:
  void Load(
      const GURL& url,
      mojo::InterfaceRequest<mojo::Application> application_request) override;

 private:
  class UILoader;

  // These functions are exected on the background thread. They call through
  // to |loader_| to do the actual loading.
  void LoadOnUIThread(
      const GURL& url,
      mojo::InterfaceRequest<mojo::Application> application_request);
  void ShutdownOnUIThread();

  scoped_ptr<ApplicationLoader> loader_;
  base::MessageLoop* ui_message_loop_;

  DISALLOW_COPY_AND_ASSIGN(UIApplicationLoader);
};

}  // namespace shell

#endif  // SHELL_ANDROID_UI_APPLICATION_LOADER_ANDROID_H_
