# mojo_debug

`mojo_debug` allows you to interactively inspect a running shell, collect
performance traces and attach a gdb debugger.

## Tracing
[Performance
traces](https://www.chromium.org/developers/how-tos/trace-event-profiling-tool)
can either be collected by Mojo Shell during its startup, or collected
interactively by `mojo_debug`.

To trace the Mojo Shell startup, use the `--trace-startup` flag:

```sh
mojo_run --trace-startup APP_URL [--android]
```

In order to collect traces interactively through `mojo_debug`, make sure that
the app being inspected was run with `--debugger` switch. E.g.:

```sh
mojo_run --debugger APP_URL [--android]
```

While Mojo Shell is running, tracing can be started and stopped by these two
commands respectively:

```sh
mojo_debug tracing start
mojo_debug tracing stop [result.json]
```

Trace files can be then loaded using the trace viewer in Chrome available at
`about://tracing`.

## GDB
It is possible to inspect a Mojo Shell process using GDB. The `mojo_debug`
script can be used to launch GDB and attach it to a running shell process
(android only):

```sh
mojo_debug gdb attach
```

Once started, GDB will first stop the Mojo Shell execution, then load symbols
from loaded Mojo applications. Please note that this initial step can take some
time (up to several minutes in the worst case).

After each execution pause, GDB will update the set of loaded symbols based on
the selected thread only. If you need symbols for all threads, use the
`update-symbols` GDB command:
```sh
(gdb) update-symbols
```

If you only want to update symbols for the current selected thread (for example,
after changing threads), use the `current` option:
```sh
(gdb) update-symbols current
```

If you want to debug the startup of your application, you can pass
`--wait-for-debugger` to `mojo_run` to have the Mojo Shell stop and wait to be
attached by `gdb` before continuing.

## Android crash stacks
When Mojo shell crashes on Android ("Unfortunately, Mojo shell has stopped.")
due to a crash in native code, `mojo_debug` can be used to find and symbolize
the stack trace present in the device log:

```sh
mojo_debug device stack
```
