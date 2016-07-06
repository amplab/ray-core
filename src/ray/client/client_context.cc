// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "client_context.h"

namespace shell {

Blocker::Blocker() : event_(true, false) {}

Blocker::~Blocker() {}

AppContext::AppContext()
    : io_thread_("io_thread"), controller_thread_("controller_thread") {}

AppContext::~AppContext() {}

void AppContext::Init(ScopedPlatformHandle platform_handle) {
  // Initialize Mojo before starting any threads.
  mojo::embedder::Init(mojo::embedder::CreateSimplePlatformSupport());

  // Create and start our I/O thread.
  base::Thread::Options io_thread_options(base::MessageLoop::TYPE_IO, 0);
  CHECK(io_thread_.StartWithOptions(io_thread_options));
  io_runner_ = MakeRefCounted<base_edk::PlatformTaskRunnerImpl>(
      io_thread_.task_runner());
  CHECK(io_runner_);
  io_watcher_ = MakeUnique<base_edk::PlatformHandleWatcherImpl>(
      static_cast<base::MessageLoopForIO*>(io_thread_.message_loop()));

  // Create and start our controller thread.
  base::Thread::Options controller_thread_options;
  controller_thread_options.message_loop_type =
      base::MessageLoop::TYPE_CUSTOM;
  controller_thread_options.message_pump_factory =
      base::Bind(&mojo::common::MessagePumpMojo::Create);
  CHECK(controller_thread_.StartWithOptions(controller_thread_options));
  controller_runner_ = MakeRefCounted<base_edk::PlatformTaskRunnerImpl>(
      controller_thread_.task_runner());
  CHECK(controller_runner_);

  mojo::embedder::InitIPCSupport(
      mojo::embedder::ProcessType::SLAVE, controller_runner_.Clone(), this,
      io_runner_.Clone(), io_watcher_.get(), platform_handle.Pass());
}

void AppContext::Shutdown() {
  Blocker blocker;
  shutdown_unblocker_ = blocker.GetUnblocker();
  controller_runner_->PostTask([this]() { ShutdownOnControllerThread(); });
  blocker.Block();
}

void AppContext::OnShutdownComplete() {
  shutdown_unblocker_.Unblock(base::Closure());
}

void AppContext::OnMasterDisconnect() {
  // We've lost the connection to the master process. This is not recoverable.
  LOG(ERROR) << "Disconnected from master";
  _exit(1);
}

ChildControllerImpl::ChildControllerImpl(AppContext* app_context,
        const Blocker::Unblocker& unblocker,
        AppInitializer app_initializer)
    : app_context_(app_context),
      unblocker_(unblocker),
      mojo_task_runner_(MakeRefCounted<base_edk::PlatformTaskRunnerImpl>(
          base::ThreadTaskRunnerHandle::Get())),
      app_initializer_(app_initializer),
      channel_info_(nullptr),
      binding_(this) {
  binding_.set_connection_error_handler([this]() { OnConnectionError(); });
}

ChildControllerImpl::~ChildControllerImpl() {
  DCHECK(thread_checker_.CalledOnValidThread());

  // TODO(vtl): Pass in the result from |MainMain()|.
  on_app_complete_.Run(MOJO_RESULT_UNIMPLEMENTED);
}

void ChildControllerImpl::Init(AppContext* app_context,
        const std::string& child_connection_id,
        const Blocker::Unblocker& unblocker,
        AppInitializer app_initializer) {
  DCHECK(app_context);
  DCHECK(!app_context->controller());

  scoped_ptr<ChildControllerImpl> impl(
      new ChildControllerImpl(app_context, unblocker, app_initializer));
  // TODO(vtl): With C++14 lambda captures, we'll be able to avoid this
  // silliness.
  auto raw_impl = impl.get();
  mojo::ScopedMessagePipeHandle host_message_pipe(
      mojo::embedder::ConnectToMaster(child_connection_id, [raw_impl]() {
        raw_impl->DidConnectToMaster();
      }, impl->mojo_task_runner_.Clone(), &impl->channel_info_));
  DCHECK(impl->channel_info_);
  impl->Bind(host_message_pipe.Pass());

  app_context->set_controller(impl.Pass());
}

void ChildControllerImpl::StartApp(const mojo::String& app_path,
        mojo::InterfaceRequest<mojo::Application> application_request,
        const StartAppCallback& on_app_complete) {
  DVLOG(2) << "ChildControllerImpl::StartApp(" << app_path << ", ...)";
  DCHECK(thread_checker_.CalledOnValidThread());

  on_app_complete_ = on_app_complete;
  unblocker_.Unblock(base::Bind(&ChildControllerImpl::StartAppOnMainThread,
                                base::FilePath::FromUTF8Unsafe(app_path),
                                base::Passed(&application_request),
                                app_initializer_));
}

void ChildControllerImpl::ExitNow(int32_t exit_code) {
  DVLOG(2) << "ChildControllerImpl::ExitNow(" << exit_code << ")";
  _exit(exit_code);
}

} // namespace shell
