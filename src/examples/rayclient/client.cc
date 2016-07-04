// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <unistd.h>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/posix/global_descriptors.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/waitable_event.h"
#include "base/thread_task_runner_handle.h"
#include "base/threading/thread.h"
#include "base/threading/thread_checker.h"
#include "mojo/edk/base_edk/platform_handle_watcher_impl.h"
#include "mojo/edk/base_edk/platform_task_runner_impl.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/multiprocess_embedder.h"
#include "mojo/edk/embedder/simple_platform_support.h"
#include "mojo/edk/embedder/slave_process_delegate.h"
#include "mojo/edk/platform/platform_handle.h"
#include "mojo/edk/platform/platform_handle_watcher.h"
#include "mojo/edk/platform/platform_pipe.h"
#include "mojo/edk/platform/scoped_platform_handle.h"
#include "mojo/edk/platform/task_runner.h"
#include "mojo/edk/util/make_unique.h"
#include "mojo/edk/util/ref_ptr.h"
#include "mojo/message_pump/message_pump_mojo.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "shell/child_controller.mojom.h"
#include "shell/child_switches.h"
#include "shell/init.h"
#include "shell/native_application_support.h"

#include "exchange_file_descriptor.h"
#include "examples/echo/echo.mojom-sync.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/bindings/synchronous_interface_ptr.h"

using mojo::platform::PlatformHandle;
using mojo::platform::PlatformHandleWatcher;
using mojo::platform::ScopedPlatformHandle;
using mojo::platform::TaskRunner;
using mojo::util::MakeRefCounted;
using mojo::util::MakeUnique;
using mojo::util::RefPtr;

namespace shell {
namespace {

/*! The ServiceConnectionApp runs in a separate thread in the client and
    maintains a connection to the shell. It allows the client to connect
    synchronously to services, one service per ServiceConnectionApp.
    It allows the client to get InterfaceHandles to these services. These
    handles can be transfered to any client thread.
*/
template<typename Service>
class ServiceConnectionApp : public mojo::ApplicationImplBase {
 public:
  /*! Construct a new ServiceConnectionApp that will connect to a service.

      \param service_name
        The name of the service we want to connect to

      \param notify_caller
        Condition that will be triggered to notify
        the calling thread that the connection to the service is established

      \param service_handle
        A pointer to an InterfaceHandle that is
        owned by the calling thread
  */
  ServiceConnectionApp(const std::string& service_name,
                           std::condition_variable* notify_caller,
                           mojo::InterfaceHandle<Service>* service_handle)
      : service_name_(service_name), notify_caller_(notify_caller),
        service_handle_(service_handle) {}

  void OnInitialize() override {
    mojo::SynchronousInterfacePtr<Service> service;
    mojo::ConnectToService(shell(), service_name_,
                           mojo::GetSynchronousProxy(&service));
    // pass handle to calling thread
    *service_handle_ = service.PassInterfaceHandle();
    notify_caller_->notify_all();
    notify_caller_ = NULL;
  }
 private:
  std::string service_name_;
  std::condition_variable* notify_caller_;
  mojo::InterfaceHandle<Service>* service_handle_;
};

// Blocker ---------------------------------------------------------------------

// Blocks a thread until another thread unblocks it, at which point it unblocks
// and runs a closure provided by that thread.
class Blocker {
 public:
  class Unblocker {
   public:
    explicit Unblocker(Blocker* blocker = nullptr) : blocker_(blocker) {}
    ~Unblocker() {}

    void Unblock(base::Closure run_after) {
      DCHECK(blocker_);
      DCHECK(blocker_->run_after_.is_null());
      blocker_->run_after_ = run_after;
      blocker_->event_.Signal();
      blocker_ = nullptr;
    }

   private:
    Blocker* blocker_;

    // Copy and assign allowed.
  };

  Blocker() : event_(true, false) {}
  ~Blocker() {}

  void Block() {
    DCHECK(run_after_.is_null());
    event_.Wait();
    if (!run_after_.is_null())
      run_after_.Run();
  }

  Unblocker GetUnblocker() { return Unblocker(this); }

 private:
  base::WaitableEvent event_;
  base::Closure run_after_;

  DISALLOW_COPY_AND_ASSIGN(Blocker);
};

// AppContext ------------------------------------------------------------------

class ChildControllerImpl;

// Should be created and initialized on the main thread.
class AppContext : public mojo::embedder::SlaveProcessDelegate {
 public:
  AppContext()
      : io_thread_("io_thread"), controller_thread_("controller_thread") {}
  ~AppContext() override {}

  void Init(ScopedPlatformHandle platform_handle) {
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

  void Shutdown() {
    Blocker blocker;
    shutdown_unblocker_ = blocker.GetUnblocker();
    controller_runner_->PostTask([this]() { ShutdownOnControllerThread(); });
    blocker.Block();
  }

  const RefPtr<TaskRunner>& controller_runner() const {
    return controller_runner_;
  }

  ChildControllerImpl* controller() const { return controller_.get(); }

  void set_controller(scoped_ptr<ChildControllerImpl> controller) {
    controller_ = controller.Pass();
  }

 private:
  void ShutdownOnControllerThread() {
    // First, destroy the controller.
    controller_.reset();

    // Next shutdown IPC. We'll unblock the main thread in OnShutdownComplete().
    mojo::embedder::ShutdownIPCSupport();
  }

  // SlaveProcessDelegate implementation.
  void OnShutdownComplete() override {
    shutdown_unblocker_.Unblock(base::Closure());
  }

  void OnMasterDisconnect() override {
    // We've lost the connection to the master process. This is not recoverable.
    LOG(ERROR) << "Disconnected from master";
    _exit(1);
  }

  base::Thread io_thread_;
  RefPtr<TaskRunner> io_runner_;
  std::unique_ptr<PlatformHandleWatcher> io_watcher_;

  base::Thread controller_thread_;
  RefPtr<TaskRunner> controller_runner_;

  // Accessed only on the controller thread.
  scoped_ptr<ChildControllerImpl> controller_;

  // Used to unblock the main thread on shutdown.
  Blocker::Unblocker shutdown_unblocker_;

  DISALLOW_COPY_AND_ASSIGN(AppContext);
};

// ChildControllerImpl ---------------------------------------------------------

typedef std::function<void(mojo::InterfaceRequest<mojo::Application>)>
  AppInitializer;

class ChildControllerImpl : public ChildController {
 public:
  ~ChildControllerImpl() override {
    DCHECK(thread_checker_.CalledOnValidThread());

    // TODO(vtl): Pass in the result from |MainMain()|.
    on_app_complete_.Run(MOJO_RESULT_UNIMPLEMENTED);
  }

  // To be executed on the controller thread. Creates the |ChildController|,
  // etc.
  static void Init(AppContext* app_context,
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

  void Bind(mojo::ScopedMessagePipeHandle handle) {
    binding_.Bind(handle.Pass());
  }

  // |ChildController| methods:
  void StartApp(const mojo::String& app_path,
                mojo::InterfaceRequest<mojo::Application> application_request,
                const StartAppCallback& on_app_complete) override {
    DVLOG(2) << "ChildControllerImpl::StartApp(" << app_path << ", ...)";
    DCHECK(thread_checker_.CalledOnValidThread());

    on_app_complete_ = on_app_complete;
    unblocker_.Unblock(base::Bind(&ChildControllerImpl::StartAppOnMainThread,
                                  base::FilePath::FromUTF8Unsafe(app_path),
                                  base::Passed(&application_request),
                                  app_initializer_));
  }

  void ExitNow(int32_t exit_code) override {
    DVLOG(2) << "ChildControllerImpl::ExitNow(" << exit_code << ")";
    _exit(exit_code);
  }

 private:
  ChildControllerImpl(AppContext* app_context,
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

  void OnConnectionError() {
    // A connection error means the connection to the shell is lost. This is not
    // recoverable.
    LOG(ERROR) << "Connection error to the shell";
    _exit(1);
  }

  // Callback for |mojo::embedder::ConnectToMaster()|.
  void DidConnectToMaster() {
    DVLOG(2) << "ChildControllerImpl::DidCreateChannel()";
    DCHECK(thread_checker_.CalledOnValidThread());
  }

  static void StartAppOnMainThread(
      const base::FilePath& app_path,
      mojo::InterfaceRequest<mojo::Application> application_request,
      AppInitializer app_initializer) {
    // TODO(vtl): This is copied from in_process_native_runner.cc.
    DVLOG(2) << "Loading/running Mojo app from " << app_path.value()
             << " out of process";

    app_initializer(application_request.Pass());
  }

  base::ThreadChecker thread_checker_;
  AppContext* const app_context_;
  Blocker::Unblocker unblocker_;
  RefPtr<TaskRunner> mojo_task_runner_;
  StartAppCallback on_app_complete_;
  AppInitializer app_initializer_;

  mojo::embedder::ChannelInfo* channel_info_;
  mojo::Binding<ChildController> binding_;

  DISALLOW_COPY_AND_ASSIGN(ChildControllerImpl);
};

}  // namespace
}  // namespace shell

template<typename Service>
std::thread start_rayclient(const std::string& address, const std::string& child_connection_id, mojo::InterfaceHandle<Service>* service) {
  std::condition_variable app_started; // signal when app was started
  std::mutex app_started_mutex; // lock for above condition
  std::thread thread([address, child_connection_id, &app_started, service]() {
    FileDescriptorReceiver receiver(address);
    int fd = receiver.Receive(); // file descriptor to bootstrap the IPC from

    mojo::platform::ScopedPlatformHandle platform_handle((mojo::platform::PlatformHandle(fd)));
    shell::AppContext app_context;
    app_context.Init(platform_handle.Pass());

    shell::Blocker blocker;
    // TODO(vtl): With C++14 lambda captures, this can be made nicer.
    const shell::Blocker::Unblocker unblocker = blocker.GetUnblocker();
    auto app_initializer = [&app_started, service](mojo::InterfaceRequest<mojo::Application> request) {
      shell::ServiceConnectionApp<Service> connector_app(std::string("mojo:echo_server"), &app_started, service);
      base::MessageLoop loop((mojo::common::MessagePumpMojo::Create()));
      connector_app.Bind(request.Pass());
      loop.Run();
    };
    app_context.controller_runner()->PostTask(
        [app_initializer, &app_context, &child_connection_id, &unblocker]() {
          shell::ChildControllerImpl::Init(&app_context, child_connection_id,
                                           unblocker, app_initializer);
        });
    // This will block, then run whatever the controller wants.
    blocker.Block();

    app_context.Shutdown();
  });
  std::unique_lock<std::mutex> lock(app_started_mutex);
  app_started.wait(lock);
  // thread.join();
  return thread;
}
