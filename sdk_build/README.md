# Tools/scripts/data for building SDKs

This directory contains (or will contain) tools, scripts, and data for building
SDKs (for various languages and platforms).

## build_sdk.py

This is a script that creates a directory containing an "SDK" using the
instructions in a given "SDK specification file" and data from the current git
repository (which should be clean; for testing purposes, you may give the
`--allow-dirty-tree` flag).

For example:

    $ ./build_sdk.py data/cpp/cpp.sdk /tmp/my_cpp_sdk

This creates an SDK for C++ in `/tmp/my_cpp_sdk`.

This script does not handle packaging such an SDK (e.g., into a tarball) or
uploading it (e.g., to Google Cloud Storage).
