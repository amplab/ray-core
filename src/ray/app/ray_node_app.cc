#include <sstream>
#include "base/logging.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/utility/run_loop.h"

#include "ray/client/client.mojom.h"

namespace ray {

/*! This is the application that runs on each Ray node and
    establishes connections to clients on that node. For now, we
    allow an arbitrary number of Python processes to be connected
    from the outside.
*/
class RayNodeApp : public mojo::ApplicationImplBase {
 public:
  void OnInitialize() override {
    for (size_t i = 0; i < 2; ++i) {
      size_t index = clients_.size();
      clients_.emplace_back();
      std::stringstream stream;
      stream << "mojo:worker{" << index << "}";
      LOG(INFO) << "Starting " << stream.str() << std::endl;
      ConnectToService(shell(), stream.str(), mojo::GetProxy(&clients_[index]));
    }
  }
 private:
  std::vector<ClientPtr> clients_;
};

}

MojoResult MojoMain(MojoHandle application_request) {
  ray::RayNodeApp ray_node_app;
  return mojo::RunApplication(application_request, &ray_node_app);
}
