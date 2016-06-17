# Moterm example application

`moterm_example_app` is an example application that embeds
[Moterm](../../apps/moterm), uses it to provide a prompt, and allows it to be
connected to other applications (which should provide the
`mojo.terminal.TerminalClient` service).

## Running

On Linux, run it in the usual way, e.g.:

    $ mojo/devtools/common/mojo_run --enable-multiprocess \
        "mojo:launcher mojo:moterm_example_app"

You'll probably need to click on the window to give it keyboard focus. You may
also want to resize the window (especially horizontally) to make it bigger.

At the `:)` prompt, you may enter the URL for any application providing the
`mojo.terminal.TerminalClient` service.

### Example 1: Dart netcat

An example of a terminal client application written in Dart is `dart_netcat`:

    :) mojo:dart_netcat

At this point, the terminal's input/output is transferred to the specified
application. In this case, it just outputs a help message and closes the
terminal, returning you to the `:)` prompt. You may also try:

    :) mojo:dart_netcat?localhost&port=80

In this case, `mojo:dart_netcat` will make a TCP connection to the specified
host/port. Assuming you have a web server running on your machine, you may try
entering:

    GET /

(It's probably a bug in `mojo:dart_netcat` that it doesn't close the terminal
"file". You can press Control-D to return to the `:)` prompt. The unhandled Dart
exception after you press Control-D is definitely a bug.)

### Example 2: Running native console applications

The `native_support` service supports running native (Linux) applications. The
`native_run_app` application provides a terminal client front-end:

    :) mojo:native_run_app

At its `>>>` prompt, you can enter name of a native application. E.g.:

    >>> bash
    $ echo hello linux
    hello linux
    $

### Example 3: JavaScript REPL

A terminal client application (written in JavaScript, using the JavaScript
content handler) that provides a JavaScript REPL:

    :) file:///path/to/src/examples/js/repl.js

At its `>` prompt, you can enter JavaScript expressions. E.g.:

    > function add(x, y) { return x + y; }
    undefined
    > add("hello ", 123)
    "hello 123"
    >

## See also

* [//apps/moterm](../../apps/moterm)
* [//examples/dart/netcat](../dart/netcat)
* [//examples/native_run_app](../native_run_app)
* [//examples/js/repl.js](../js/repl.js)
