Dart Echo Client Example
========================

This is the Dart version of the canonical Echo Client sample code for Mojo. It
starts up the dart_echo_server service and calls the EchoString() interface in
that server.  The expected result is that it prints `hello world`.

The source code to the Dart version of the Echo Server can be found in
[`mojo/src/benchmarks/mojo_rtt_benchmark/lib/echo_server.dart`]
(https://github.com/domokit/mojo/blob/master/benchmarks/mojo_rtt_benchmark/lib/echo_server.dart).

The tutorial for this code can be found at:
https://docs.google.com/document/d/1mufrtxTk8w9qa3jcnlgqsYkWlyhwEpc7aWNaSOks7ug/edit?usp=sharing

## Running the code

Run using the Dart echo server, named `dart_echo_server.mojo` (this name is
hardcoded into the client sources as the default server):
```
$ out/Debug/mojo_shell mojo:dart_echo_client
```

Run using the C++ echo server to show that any server can implement an interface.
Note that .mojo is left off:
```
$ out/Debug/mojo_shell "--args-for=mojo:dart_echo_client echo_server" mojo:dart_echo_client
```
