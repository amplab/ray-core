# Moterm

`moterm` is an application that provides an embeddable view, which provides a
VT100-style terminal that its embedder can connect to other applications. It is
not useful as a standalone application (embedded by something, like a window
manager, that is not aware of how to control it).

For an example of an embedder that's useful as a standalone application, see the
[Moterm example application](../../examples/moterm_example_app).

## Details

`moterm` exposes the `mojo.terminal.Terminal` interface to its embedder. Via
this interface, its embedder may connect to the terminal directly (for direct
input/output with the terminal) or request that the terminal connect to another
application. In the latter case, that application should implement the
`mojo.terminal.TerminalClient` interface.

Output to/input from the terminal is done via a `mojo.files.File` that behaves
"like a terminal" (in much the same way as in Unix one has terminal device
files and file descriptors to them).

## See also

* [//examples/moterm_example_app](../../examples/moterm_example_app)
* [//mojo/services/files/interfaces](../../mojo/services/files/interfaces)
* [//mojo/services/terminal/interfaces](../../mojo/services/terminal/interfaces)
