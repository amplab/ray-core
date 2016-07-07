#include "plasma_interface.h"

namespace plasma {

PlasmaInterface::PlasmaInterface(const std::string& address) {
    context_.ConnectToShell(std::string("mojo:plasma"), std::string(address));
    interface_ = context_.GetInterface();
}

PlasmaInterface::~PlasmaInterface() {}

mojo::SynchronousInterfacePtr<plasma::service::Plasma>& PlasmaInterface::get() {
  return interface_;
}

}
