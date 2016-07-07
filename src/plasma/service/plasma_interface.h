#ifndef PLASMA_INTERFACE_H_
#define PLASMA_INTERFACE_H_

#include <string>
#include "mojo/public/cpp/bindings/synchronous_interface_ptr.h"
#include "plasma/service/plasma.mojom-sync.h"
#include "ray/client/client_context.h"

namespace plasma {

/*! This class holds all the fields necessary to interact with the Plasma
    service. They are collected here so that the public API headers do not
    depend on Mojo (this is the "pointer to implementation" pattern).
*/
class PlasmaInterface {
 public:
  PlasmaInterface(const std::string& address);
  ~PlasmaInterface();
  mojo::SynchronousInterfacePtr<plasma::service::Plasma>& get();
 private:
  mojo::SynchronousInterfacePtr<plasma::service::Plasma> interface_;
  shell::ClientContext<plasma::service::Plasma> context_;
};

}

#endif
