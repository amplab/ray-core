Quick Start Guide
=================

To build Ray Core, execute the following commands:

First, install the requirements:

::

  sudo apt-get update
  sudo apt-get install git subversion build-essential
  sudo apt-get python-dev g++-multilib libcap-dev

Then, install depot_tools:

::

  git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
  export PATH=`pwd`/depot_tools:"$PATH"

Check out and build the project:

::

  git clone https://github.com/amplab/ray-core
  cd ray-core
  glient sync
  cd src
  gn gen out/Debug
  ninja -C out/Debug -j 16

To make sure everything works, you can try out the Mojo hello world example:

::

  cd ray-core/src/out/Debug
  ./mojo_shell mojo:hello_mojo_client

Now, the Ray shell can be started with

::

  cd ray-core/src/out/Debug
  ./mojo_shell --enable-multiprocess
               --external-connection-address=/home/ubuntu/shell
               ray_node_app.mojo
