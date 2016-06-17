# Native run example application

`native_run_app` is a terminal client application (i.e., one providing the
`mojo.terminal.TerminalClient` service) that allows native (Linux) applications
to be run inside a Mojo ([Moterm](../../apps/moterm)) terminal. It does this by
using the `native_support` service, which provides facilities to
(programmatically) run native applications.

See [//examples/moterm_example_app](../moterm_example_app) for an example of how
it may be used.

## See also

* [//apps/moterm](../../apps/moterm)
* [//services/native_support](../../services/native_support)
