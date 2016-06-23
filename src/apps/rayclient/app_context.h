#ifndef RAYCLIENT_APP_CONTEXT_H
#define RAYCLIENT_APP_CONTEXT_H

#include "base/threading/thread.h"
#include "mojo/edk/base_edk/platform_handle_watcher_impl.h"
#include "mojo/edk/base_edk/platform_task_runner_impl.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/multiprocess_embedder.h"
#include "mojo/edk/embedder/simple_platform_support.h"
#include "mojo/edk/embedder/slave_process_delegate.h"
#include "mojo/edk/platform/scoped_platform_handle.h"
#include "mojo/edk/util/make_unique.h"
#include "mojo/edk/util/ref_ptr.h"
#include "mojo/message_pump/message_pump_mojo.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "shell/child_controller.mojom.h"
#include "base/files/file_path.h"
#include "base/threading/thread_checker.h"
#include "base/thread_task_runner_handle.h"

#include "blocker.h"

using mojo::util::MakeRefCounted;
using mojo::util::MakeUnique;
using mojo::platform::PlatformHandle;
using mojo::platform::PlatformHandleWatcher;
using mojo::util::RefPtr;
using mojo::platform::ScopedPlatformHandle;
using mojo::platform::TaskRunner;

namespace shell {

class ChildControllerImpl;

// Should be created and initialized on the main thread.
class AppContext : public mojo::embedder::SlaveProcessDelegate {
 public:
  AppContext();

  ~AppContext() override;

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
  void OnShutdownComplete() override;

  void OnMasterDisconnect() override;

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

inline AppContext::AppContext()
      : io_thread_("io_thread"), controller_thread_("controller_thread") {}

inline AppContext::~AppContext() {}

inline void AppContext::OnShutdownComplete() {
  shutdown_unblocker_.Unblock(base::Closure());
}

inline void AppContext::OnMasterDisconnect() {
  // We've lost the connection to the master process. This is not recoverable.
  LOG(ERROR) << "Disconnected from master";
  _exit(1);
}

}  // namespace shell

#endif // RAYCLIENT_APP_CONTEXT_H
