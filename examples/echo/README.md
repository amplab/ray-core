Example Echo Client & Server
====

This echo client/server demonstrate how to create and use a mojom interface,
as well as demonstrating one way to communicate between mojo applications.

For a deeper dive into this code, refer to the [Mojo
Tutorial](https://docs.google.com/document/d/1mufrtxTk8w9qa3jcnlgqsYkWlyhwEpc7aWNaSOks7ug).

## Running the Echo Client & Server

```
$ ./mojo/tools/mojob.py gn
$ ./mojo/tools/mojob.py build
$ ./out/Debug/mojo_shell mojo:echo_client
```
You should see output along the lines of:

```
[1010/194919:INFO:echo_client.cc(21)] ***** Response: hello world
```

This means that our echo_client started, contacted the echo_server (which was
started by the shell), sent a string through a mojom interface, and got a
response.

## Echo Client Structure

By running the echo_client through mojo_shell, we run a [Mojo
Application](https://docs.google.com/document/d/1xjt_TPjTu0elix8fNdBgWmnjJdJAtqSr1XDS_C-Ct8E).
Mojo Applications have main threads (run as `MojoMain`), and they may
communicate with other applications using Mojo IPC. This section will describe
the steps taken to start up the echo client, and what is necessary
to make an IPC call to the echo server service.

### ApplicationRunner: It calls your application

In echo_client.cc's MojoMain function, a new `ApplicationRunner` called `runner`
is created. This class is used by the shell for setting up and communicating
with an application. This is a common pattern in Mojo apps: a runner takes an
implementation of an `ApplicationDelegate` and runs it.

In echo_client, the delegate is a class called `EchoClientDelegate`.

### ApplicationDelegate: Your application will be a subclass of this

The ApplicationDelegate class is what we can think of as the heart of our
"app". It can implement three methods:

1. `void Initialize(ApplicationImpl* app)` -- Called during setup.
2. `bool ConfigureIncomingConnection(ApplicationConnection* connection)` --
   Configures what happens when a connection attempts to reach our application.
3. `void Quit()` -- Called before termination.

Our echo_client only implements the `Initialize` method, since it does not need
to accept any incoming connections (it just makes one outgoing connection).

This initialize method takes an `ApplicationImpl* app` as an argument, which can
be used to contact other services. Here, we contact the "mojo:echo_server"
service using the `ConnectToService` method. This method takes a URL as an
argument, and passes an interface back in an `InterfacePtr`.

```
app->ConnectToService("mojo:echo_server", &echo_);
```

Note: When the Mojo Shell notices the echo_server service is not running, it
will automatically start the server service. This is why only running the client
is necessary for this example to work.

### Mojom Interfaces: An mechanism for predictable message passing

Interfaces are defined in ".mojom" files, and they allow applications to
interact with each other in a procedure-call mechanism. In mojom interfaces,
a client invokes a method, the arguments are serialized and passed to the
receiver, and the receiver invokes the method (and returns any results).

The [Mojom
language](https://docs.google.com/document/d/1r7yCseBktlDEN9CKp_JWD0ZYxMi4GCsLXMvSN5sI04k)
is used to define the simple `EchoString` interface, defined in echo.mojom.  To
compile the mojom interface, it must be built using the "mojom" template in a
BUILD.gn file. The "echo.mojom" file is compiled as a part of a target named
"bindings". This will autogenerate a few files, one of which we are including in
our echo_client.cc example: "examples/echo/echo.mojom.h". Since our mojom file
specifies `interface Echo`, we can refer to the `EchoPtr` type to access our
interface.

If you create an `interface FooBar`, then you can use a type `FooBarPtr` to
reference your interface.

Since our interface defines the method:

```
EchoString(string? value) => (string? value)
```

this creates the following method (and more code, not shown):

```
void EchoString(const mojo::String& value, mojo::Callback<void(mojo::String)>);
```

This method is callable on an `InterfacePtr` which properly implements our mojom
interface (so, in this case, an `EchoPtr`, like the one returned from our
call to `ConnectToService`).

The second argument to our interface is a `mojo::Callback` class, which is
just a Runnable with varying arguments.  For the echo_client example, we created
a `ResponsePrinter` class to act as this callback. By implementing `Run`, which
merely prints out the string we get from the echo server, we are able to call
the `EchoString` method on the `EchoPtr` received from `ConnectToService`.

```
echo_->EchoString("hello world", ResponsePrinter());
```

In summary, the echo_client connects to the echo_server service using
the `ConnectToService` method, passing an interface defined in a mojom file. The
methods of this interface can then be invoked on the `EchoPtr`, with appropriate
callback implementations being passed where necessary.

## Echo Server Structure

The echo server, like the echo client, is implemented as an application. This
means it has a `MojoMain` function, an `ApplicationRunner`, and an
`ApplicationDelegate` actually implementing the core application.

echo_server.cc contains three different types of servers, though only one can be
used at a time. To try changing the server, uncomment one of the lines in
MojoMain. These different `ApplicationDelegate` derivations demonstrate
different ways in which incoming requests can be handled.

All three servers, being `ApplicationDelegate` derivations, implement
`ConfigureIncomingConnection` in the same way:

```
service_provider_impl->AddService<Echo>(
    [this](const mojo::ConnectionContext& connection_context,
           mojo::InterfaceRequest<Echo> echo_request) {
      ...
    });
```

This should be read as "For any incoming connections to this server, use the
given lambda function use `this` to create the Echo interface".

### EchoImpl: The Interface Implementation

All three implementations use the `EchoImpl` class, implementing the `Echo`
interface we defined in our mojom file, which does what you would expect of an
echo server: it sends back the supplied value String back to the client.

```
callback.Run(value);
```

If we wanted the server to pass back something else, we would pass a different
value here. However, as defined by our interface, the echo server can only
return a String.

### Server 1: MultiServer

On calls to `Create`, this server creates a new `StrongBindingEchoImpl` object
for each request.  This object is derived from `EchoImpl`, so it implements the
interface, but by using the `StrongBinding` class, it cleans up after itself
once the message pipe used for the request is closed.

### Server 2: SingletonServer

This server creates an `EchoImpl` object when it is constructed, and for each
call to `Create`, binds the request to this single implementation. A
`BindingSet` is used so that multiple requests can be bound to the same object.

### Server 3: OneAtATimeServer

This server creates an `EchoImpl` object, like the `SingletonServer`, but uses a
single `Binding`, rather than a `BindingSet`. This means that when a new client
connects to the OneAtATimeServer, the previous binding is closed, and a new
binding is made between the new client and the interface implementation.

The OneAtATimeServer demonstrates a pattern that should be avoided because it
contains a race condition for multiple clients.  If a new client binds to the
server before the first client managed to call EchoString, the first client's
call would cause an error. Unless you have a specific use case for this
behavior, it is advised to avoid creating a server like this.
