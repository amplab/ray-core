# The Ray Core

## Building

There are two modes the core can be built in: Standalone mode and chromium mode. 

### Standalone mode

In standalone mode, the core can be built with just a compiler and a few small
dependencies installed. It is the recommended way if you want to build the core
to use its components or if you want to make small modifications to the source
code.

To build the core in standalone mode, run

```bash
git clone https://github.com/amplab/ray-core
cd ray-core
./install-deps.sh # Install dependencies. This needs root access, you can also run the commands by hand
./build-thirdparty.sh # Build thirdparty libraries, this does not require root access
cd src
../third_party/gn gen out/Debug --args='standalone_build=true' # Generate build files.
../third_party/ninja -C out/Debug -j 16 # Build ray core.
```

### Chromium mode

Chromium mode will use the full chromium build infrastructure. This is
recommended if you work on more substantial changes of the core. It will give
you a modified compiler with better diagnostics, a memory checker and the
ability to recreate mojo interface definitions.

Here is how to build the core in chromium mode.

```bash
sudo apt-get update && sudo apt-get install git subversion build-essential python-dev g++-multilib libcap-dev
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH=`pwd`/depot_tools:"$PATH"
git clone https://github.com/amplab/ray-core
cd ray-core
gclient sync
cd src
gn gen out/Debug --args='standalone_build=false'
ninja -C out/Debug -j 16
```
