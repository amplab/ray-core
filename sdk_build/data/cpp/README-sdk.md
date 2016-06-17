# Mojo C++ SDK

This is a minimal SDK for writing/building Mojo applications/services in C++.
Currently, it supports building for Linux. (**TODO(vtl)**: Support other
configurations/targets.)

It includes a simple Makefile that demonstrates building a couple of simple
example applications/services (in `examples/hello_mojo`). Note that the Makefile
is intended to be illustrative, and not for maximal correctness or practicality
(for example, it does not deal with dependencies correctly).

Users of the SDK are expected to set up builds in an appropriate way for their
situation, but the Makefile should show the key steps (in particular, generating
the C++ source files from Mojom files).

## Prerequisites

To fetch the Mojom tool and clang:
* bash
* python 2.7

To build the examples:
* binutils
* make

## Set up

To get started quickly, just run the following:

    $ mojo_sdk_setup/setup.sh

This is equivalent to performing the steps below separately:

    $ mojo_sdk_setup/download_mojom_tool.sh
    $ mojo_sdk_setup/download_clang.sh

The first downloads a binary for the Mojom tool, which is needed to generate
code from Mojom files. The second downloads a suitable version of clang, which
is needed by the included Makefile. (One should also be able to build using
another compiler with sufficient C++11 support.)

## Building the examples

To build the included examples:

    $ make -j8

(or similar). All the build output is put into the `out` subdirectory. The
examples themselves are `hello_mojo_client.mojo` and `hello_mojo_server.mojo`.

## See also

* [The Mojo project on GitHub](https://github.com/domokit/mojo)
* [Mojo documentation](https://github.com/domokit/mojo/tree/master/docs)
