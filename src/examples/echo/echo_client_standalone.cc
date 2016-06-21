#include "base/at_exit.h"
#include "base/command_line.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/simple_platform_support.h"

int main(int argc, char** argv) {
  base::AtExitManager exit_manager;
  base::CommandLine::Init(argc, argv);

  mojo::Environment env;
  mojo::embedder::Init(mojo::embedder::CreateSimplePlatformSupport());
}
