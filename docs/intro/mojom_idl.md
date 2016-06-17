# Mojom IDL

The Mojom IDL (interface definition language) is primarily used to describe
*interfaces* to be used on [message pipes](message_pipes.md). Below, we give a
brief overview of some practical aspects of the Mojom language (for more
details, see the [Mojom language](../mojom_lang/mojom_lang.md). Elsewhere, we
describe the [Mojom protocol](mojom_protocol.md). (**TODO(vtl)**: Also,
serialization format? Versioning?)

Text files written in Mojom IDL are given the `.mojom` extension by convention
(and are usually referred to as Mojom/mojom/`.mojom` files). Mojom IDL permits
C++-style comments: single-line comments starting with `//` or multi-line
comments enclosed by `/* ... */`.

The Mojom bindings generator (**TODO(vtl)**: link?) may be used to generate code
in a variety of languages (including C++, Dart, and Go) from a Mojom file. Such
generated code "implements" the things specified in the Mojom file, in a way
that's appropriate for the particular target language.

## Modules and imports

A Mojom file begins with an optional *module* declaration, which acts like a C++
namespace declaration (applying to the entire file). It is then followed by zero
or more *import* statements, which make the contents of the imported files (and,
transitively, their imports) available in the current file. For example:
```mojom
module my_module.my_submodule;

import "path/to/another.mojom";
import "path/to/yet/a/different.mojom";
```

### Name resolution

Name resolution is basically C++-like (with `.` instead of `::`): Within
`my_module.my_submodule`, an unnested declaration of a name `Foo` declares
something with "full" name `my_module.my_submodule.Foo`. A *use* of a name `Foo`
could either refer to one of the "full" names: `my_module.my_submodule.Foo`,
`my_module.Foo`, or `Foo` (searched in that order).

Nested declarations act in the expected way. E.g., if `Foo` is a struct
containing an enum declaration of `Bar`, then `Foo.Bar` (or
`my_submodule.Foo.Bar`, or `my_module.my_submodule.Foo.Bar`) can be used to
refer to that enum outside of `Foo`.

### Names and ordinals

Generally, at a binary (as opposed to source) level, names in Mojom are not
important (except in that they must not collide). Names may be changed without
affecting binary compatibility.

Instead, what's important are *ordinals*, which apply to struct fields
(including message request/response parameters) and interface messages. Often,
these are left implicit, in which case ordinals are assigned consecutively
starting from 0. (Obviously, with implicit declaration, the order of declaration
of struct fields, etc. is important.) Ordinals may also be assigned explicitly,
using the notation `@123` (for example). (This allows struct fields, etc. to be
re-ordered in a Mojom file without breaking binary compatibility.)

Though ordinals are important for evolving Mojom files in a backwards-compatible
way, we will not discuss them in this introduction.

### Naming style

Though names are not important, various code generators expect names in a
certain *style*, in order to transform them into a style more appropriate for a
given target language:
* `StudlyCaps` (a.k.a. `CapitalizedCamelCase`) for: (struct, interface, union,
  and enum) type names and message (a.k.a. function or method) names;
* `unix_hacker_style` for field names (in structs and unions) and "parameter"
  names;
* `ALL_CAPS_UNIX_HACKER_STYLE` for enum value names; and
* `kStudlyCaps` for const names.

Following this style is highly recommended (and may be required as a practical
matter).

## Interfaces

A Mojom *interface* is (typically) used to describe communication on a message
pipe. Typically, message pipes are created with a particular interface in mind,
with one endpoint designated the *client* (which sends *request* messages and
receives *response* messages) and the other designated the *server* or *impl*
(which receives request messages and sends response messages).

For example, take the following Mojom interface declaration:
```mojom
interface MyInterface {
  Foo(int32 a, string b);
  Bar() => (bool x, uint32 y);
  Baz() => ();
};
```
This specifies a Mojom interface in which the client may send three types of
messages, namely `Foo`, `Bar`, and `Baz` (see the note below about names in
Mojom). The first does not have a response message defined, whereas the latter
two do. Whenever the server receives a `Bar` or `Baz` message, it *must*
(eventually) send a (single) corresponding response message.

The `Foo` request message contains two pieces of data: a signed (two's
complement) 32-bit integer called `a` and a Unicode string called `b`. On the
"wire", the message basically consists of metadata and a (serialized) *struct*
(see below) containing `a` and `b`.

The `Bar` request message contains no data, so on the wire it's just metadata
and an empty struct. It has a response message, containing a boolean value `x`
and an unsigned 32-bit integer `y`, which on the wire consists of metadata and a
struct with `x` and `y`. Each time the server receives a `Bar` message, it is
supposed to (eventually) respond by sending the response message. (Note: The
client may include as part of the request message's metadata an identifier for
the request; the response's metadata will then include this identifier, allowing
it to match responses to requests.)

The `Baz` request message also contains no data. It requires a response, also
containing no data. Note that even though the response has no data, a response
message must nonetheless be sent, functioning as an "ack". (Thus this is
different from not having a response, as was the case for `Foo`.)

## Structs

Mojom defines a way of serializing data structures (with the Mojom IDL providing
a way of specifying those data structures). A Mojom *struct* is the basic unit
of serialization. As we saw above, messages are basically just structs, with a
small amount of additional metadata.

Here is a simple example of a struct declaration:
```mojom
struct MyStruct {
  int32 a;
  string b;
};
```
Structs (and interfaces) may also contain enum and const declarations, which
we will discuss below.

## Types

### Non-reference (simple and enum) types

We have seen some simple types above, namely `int32`, `uint32`, and `bool`. A
complete list of *simple* types is:
* `bool`: boolean values;
* `int8`, `int16`, `int32`, `int64`: signed 2's-complement integers of the given
  size;
* `uint8`, `uint16`, `uint32`, `uint64`: unsigned integers of the given size;
  and
* `float`, `double`: single- and double-precision IEEE floating-point numbers.

Additionally, there are *enum* types, which are user-defined. Internally, enums
are signed 2's complement 32-bit integers, so their values are restricted to
that range. For example:
```mojom
enum MyEnum {
  ONE_VALUE = 1,
  ANOTHER_VALUE = -5,
  THIRD_VALUE,  // Implicit value of -5 + 1 = -4.
  A_DUPLICATE_VALUE = THIRD_VALUE,
};
```
Such an enum type may be used in a struct. For example:
```mojom
struct AStruct {
  MyEnum x;
  double y;
};
```
(As previously mentioned, an enum declaration may be nested inside a struct or
interface declaration.)

Together, the simple and enum types comprise the *non-reference* types. The
remaining types are the *reference* types: *pointer* types and *handle* types.
Unlike the non-reference types, the reference types all have some notion of
"null".

### Pointer types

A struct is itself a pointer type, and can be used as a member of another struct
(or as a request/response parameter, for that matter). For example:
```mojom
struct MyStruct {
  int32 a;
  string b;
};

struct MySecondStruct {
  MyStruct x;
  MyStruct? y;
};
```
Here, `x` is a *non-nullable* (i.e., required) field of `MySecondStruct`,
whereas `y` is *nullable* (i.e., optional).

A complete list of pointer types is:
* structs: structs, as discussed above;
* `string`/`string?`: Unicode strings;
* `array<Type>`/`array<Type>?`: variable-size arrays (a.k.a. vectors or lists)
  of type "Type" (which may be any type);
* `array<Type, n>`/`array<Type, n>?`: fixed-size arrays of type "Type" and size
  "n";
* `map<KeyType, ValueType>`/`map<KeyType, ValueType>?`: maps (a.k.a.
  dictionaries) of key type "KeyType" (which may be any non-reference type or
  `string`) and value type "ValueType" (which may be any type); and
* unions: see below.

#### Unions

*Unions* are "tagged unions". Union declarations look like struct declarations,
but with different meaning. For example:
```mojom
union MyUnion {
  int32 a;
  int32 b;
  string c;
};
```
An element of type `MyUnion` must contain either an `int32` (called `a`), an
`int32` (called `b`), *or* a string called `c`. (Like for structs, `MyUnion z`
indicates a non-nullable instance, and `MyUnion?` indicates a nullable instance;
in the nullable case, `z` may either be null or it must contain one of `a`, `b`,
or `c`.)

### Handle types

#### Raw handle types

There are the "raw" *handle* types corresponding to different [Mojo
handle](handles.md) types, with mostly self-explanatory names: `handle` (any
kind of Mojo handle), `handle<message_pipe>`, `handle<data_pipe_consumer>`,
`handle<data_pipe_producer>`, and `handle<shared_buffer>`. These are used to
indicate that a given message or struct contains the indicated type of Mojo
handle (recall that messages sent on [Mojo message pipes](message_pipes.md) may
contain handles in addition to simple data).

Like the pointer types, these may also be nullable (e.g., `handle?`,
`handle<message_pipe>?`, etc.), where "null" indicates that no handle is to be
sent (and may be realized, e.g., as the invalid Mojo handle).

#### Interface types

We have already seen *interface* type declarations. In a message (or struct), it
is just a message pipe (endpoint) handle. However, it promises that the *peer*
implements the given interface. For example:
```mojom
interface MyFirstInterface {
  Foo() => ();
};

interface MySecondInterface {
  Bar(MyFirstInterface x);
  Baz(MyFirstInterface& y);  // Interface request! See below.
};
```
Here, a receiver of a `Bar` message is promised a message pipe handle on which
it can send (request) messages from `MyFirstInterface` (and then possibly
receive responses). I.e., on receiving a `Bar` message, it may then send `Foo`
message on `x` (and then receive the response to `Foo`).

Like other handle types, instances may be non-nullable or nullable (e.g.,
`MyFirstInterface?`).

#### Interface request types

*Interface request* types are very much like interface types, and also arise
from interface type declarations. They are annotated by a trailing `&`: e.g.,
`MyFirstInterface&` (or `MyFirstInterface&?` for the nullable version).

In a message (or struct), an interface request is also just a message pipe
handle. However, it is a promise/"request" that the given message pipe handle
implement the given interface (in contrast with the peer implementing it).

In the above example, the receiver of `Baz` also gets a message pipe handle.
However, the receiver is expected to implement `MyFirstInterface` on it (or pass
it to someone else who will do so). I.e., `Foo` may be *received* on `y` (and
then the response sent on it).

## Pipelining

We saw above that Mojom allows both "interfaces" and "interface requests" to be
sent in messages. Consider the following interface:
```mojom
interface Foo {
  // ...
};

interface FooFactory {
  CreateFoo1() => (Foo foo);
  CreateFoo2(Foo& foo_request);
};
```

`CreateFoo1` and `CreateFoo2` are functionally very similar: in both cases, the
sender will (eventually) be able to send `Foo` messages on some message pipe
handle. However, there are some important differences.

In the case of `CreateFoo1`, the sender is only able to do so upon receiving the
response to `CreateFoo1`, since the message pipe handle to which `Foo` messages
can be written is contained in the response message to `CreateFoo1`.

For `CreateFoo2`, the operation is somewhat different. Before sending
`CreateFoo2`, the sender creates a message pipe. This consists of two message
pipe handles (for peer endpoints), which we'll call `foo` and `foo_request` (the
latter of which will be sent in the `CreateFoo2` message). Since message pipes
are asynchronous and buffered, the sender can start writing `Foo` messages to
`foo` at any time, possibly even before `CreateFoo2` is sent! I.e., it can use
`foo` without waiting for a response message. This is referred to as
*pipelining*.

Pipelining is typically more efficient, since it eliminates the need to wait for
a response, and it is often more natural, since receiving the response often
entails returning to the message loop. Thus this is generally the preferred
pattern for "factories" as in the above example.

The main caveat is that with pipelining, there is no flow control. The sender of
`CreateFoo2` has no indication of when `foo` is actually "ready", though even in
the case of `CreateFoo1` there is no real promise that the `foo` in the response
is actually "ready". (This is perhaps an indication that flow control should be
done on `Foo`, e.g., by having its messages have responses.)

Relatedly, with pipelining, there is limited opportunity to send back
information regarding `foo`. (E.g., the preferred method of signalling error is
to simply close `foo_request`.) So if additional information is *needed* to make
use of `foo`, perhaps the pattern of `CreateFoo1` is preferable, e.g.:
```mojom
  CreateFoo() => (Foo foo, NeededInfo info);
```

## Consts

Mojom supports "constants" to be declared, mainly to provide a way of defining
semantically significant values to be used in messages, structs, etc. For
example:
```mojom
const int32 kZero = 0;
const bool kVeryTrue = true;
const double kMyDouble = 123.456;
const string kMyString = "my string";

enum MyEnum {
  ZERO,
  ONE,
  TWO,
};
const MyEnum kMyEnumValue = TWO;
```

The type may be any non-reference type (including enum types; see above) or
string. The value must be appropriate (e.g., in range) for the given type.
(There is additional syntax for doubles and floats: `double.INFINITY`,
`double.NEGATIVE_INFINITY`, `double.NAN`, and similarly for floats.)

Const declarations may be made at the top level, or they may be nested within
interface and struct declarations.

## Annotations

Various elements in Mojom files may have (optional) *annotations*. These are
lists of key-value pairs, containing "secondary" information. For example:
```mojom
[DartPackage="foobar",
 JavaPackage="com.example.mojo.foobar"]
module foobar;
```
This is an annotation attached to the `module` keyword with two key-value pairs
(one to be used by the Dart language generator and the other by the Java
language generator, respectively).

Apart from language-specific annotations, one noteworthy annotation is the
`ServiceName` annotation (for interfaces):
```mojom
[ServiceName="foobar.MyInterface"]
interface MyInterface {
  // ...
};
```
This indicates the standard name to use in conjunction with
`mojo.ServiceProvider.ConnectToService()` (**TODO**(vtl): need a reference for
that).

Annotations are also used for *versioning*, but we will not discuss that here.

## See also

**TODO(vtl)**
