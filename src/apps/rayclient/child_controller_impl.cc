#include "child_controller_impl.h"

void mojo::apps::ConnectApp::OnInitialize() {
  Terminate(MOJO_RESULT_OK);
}

namespace shell {

ChildControllerImpl::~ChildControllerImpl() {
  DCHECK(thread_checker_.CalledOnValidThread());

  // TODO(vtl): Pass in the result from |MainMain()|.
  on_app_complete_.Run(MOJO_RESULT_UNIMPLEMENTED);
}

void ChildControllerImpl::ExitNow(int32_t exit_code) {
  DVLOG(2) << "ChildControllerImpl::ExitNow(" << exit_code << ")";
  _exit(exit_code);
}

void ChildControllerImpl::StartApp(const mojo::String& app_path,
              mojo::InterfaceRequest<mojo::Application> application_request,
              const ChildControllerImpl::StartAppCallback& on_app_complete) {
  DVLOG(2) << "ChildControllerImpl::StartApp(" << app_path << ", ...)";
  DCHECK(thread_checker_.CalledOnValidThread());

  on_app_complete_ = on_app_complete;
  unblocker_.Unblock(base::Bind(&ChildControllerImpl::StartAppOnMainThread,
                                base::FilePath::FromUTF8Unsafe(app_path),
                                base::Passed(&application_request)));
}

}  // namespace shell
