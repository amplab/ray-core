# Devtools

Unopinionated tools for **running**, **debugging**, **testing** and
**benchmarking** Mojo apps.

## Install

```
git clone https://github.com/domokit/devtools.git
```

## Contents

Devtools offers the following tools:

 - `mojo_run` - [documentation](docs/mojo_run.md) - shell runner
 - `mojo_debug` - [documentation](docs/mojo_debug.md) - debugger
 - `mojo_test` - apptest runner
 - `mojo_benchmark` - [documentation](docs/mojo_benchmark.md) - perf test runner

Additionally, `remote_adb_setup` script helps to configure adb on a remote
machine to communicate with a device attached to a local machine, forwarding the
ports used by `mojo_run`.

## Development

The library is canonically developed [in the mojo
repository](https://github.com/domokit/mojo/tree/master/mojo/devtools/common),
https://github.com/domokit/devtools is a mirror allowing to consume it
separately.
