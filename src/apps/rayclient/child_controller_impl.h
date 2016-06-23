#ifndef RAYCLIENT_CHILD_CONTROLLER_IMPL_H
#define RAYCLIENT_CHILD_CONTROLLER_IMPL_H

#include "app_context.h"
#include "blocker.h"

namespace mojo {
namespace apps {

class ConnectApp : public ApplicationImplBase {
 public:
  void OnInitialize() override;
};

}  // namespace apps
}  // namespace mojo

namespace shell {

// ChildControllerImpl ---------------------------------------------------------

class ChildControllerImpl : public ChildController {
 public:
  ~ChildControllerImpl() override;

  // To be executed on the controller thread. Creates the |ChildController|,
  // etc.
  static void Init(AppContext* app_context,
                   const std::string& child_connection_id,
                   const Blocker::Unblocker& unblocker) {
    DCHECK(app_context);
    DCHECK(!app_context->controller());

    scoped_ptr<ChildControllerImpl> impl(
        new ChildControllerImpl(app_context, unblocker));
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
                const StartAppCallback& on_app_complete) override;

  void ExitNow(int32_t exit_code) override;

 private:
  ChildControllerImpl(AppContext* app_context,
                      const Blocker::Unblocker& unblocker);

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
      mojo::InterfaceRequest<mojo::Application> application_request) {
    // TODO(vtl): This is copied from in_process_native_runner.cc.
    DVLOG(2) << "Loading/running Mojo app from " << app_path.value()
             << " out of process";

    mojo::apps::ConnectApp connect_app;
    std::unique_ptr<base::MessageLoop> loop(new base::MessageLoop(mojo::common::MessagePumpMojo::Create()));
    connect_app.Bind(application_request.Pass());
    loop->Run();

    // mojo::SynchronousInterfacePtr<mojo::examples::Echo> echo;
    // mojo::ConnectToService(echo_client_app.shell(), "mojo:objstore_server", mojo::GetSynchronousProxy(&echo));
  }

  base::ThreadChecker thread_checker_;
  AppContext* const app_context_;
  Blocker::Unblocker unblocker_;
  RefPtr<TaskRunner> mojo_task_runner_;
  StartAppCallback on_app_complete_;

  mojo::embedder::ChannelInfo* channel_info_;
  mojo::Binding<ChildController> binding_;

  DISALLOW_COPY_AND_ASSIGN(ChildControllerImpl);
};

inline ChildControllerImpl::ChildControllerImpl(AppContext* app_context,
                    const Blocker::Unblocker& unblocker)
  : app_context_(app_context),
    unblocker_(unblocker),
    mojo_task_runner_(MakeRefCounted<base_edk::PlatformTaskRunnerImpl>(
        base::ThreadTaskRunnerHandle::Get())),
    channel_info_(nullptr),
    binding_(this) {
  binding_.set_connection_error_handler([this]() { OnConnectionError(); });
}

}  // namespace shell

#endif // RAYCLIENT_CHILD_CONTROLLER_IMPL_H
