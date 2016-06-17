# Mojom language reference

This is a reference for the Mojom interface definition language (IDL). See
[Mojom IDL](../intro/mojom_idl.md) for a shorter introduction.

The Mojom language is ultimately about defining *types* (and things associated
to types), including in particular *interface* types (hence "interface
definition language"). It also allows "constant" values to be defined for some
simple types, though this is mostly in support of the former role.

## Mojom files

Mojom files are Unicode text files, encoded in UTF-8. *Whitespace* (spaces,
tabs, newlines, carriage returns) is not significant in Mojom files, except
insofar as they separate tokens. Thus any consecutive sequence of whitespace
characters may be replaced by a single whitespace character without any semantic
change.

### Comments

There are two kinds of *comments*. Both are ignored, except that they too
separate tokens (so any comment may be replaced by a single whitespace
character).

The first is the *single-line* (C++-style) comment. It is started by a `//`
outside of a *string literal* and outside another comment and terminated by a
newline. For example:
```mojom
// This is a comment.

interface// This "separates" tokens.
AnInterface {};

const string kAConstString = "// This is not a comment.";

[AnAttribute="// This is also not a comment either."]
interface AnotherInterface {};
```

The second is the *multi-line* (C-style) comment. It is started by a `/*`
outside of a string literal and terminated by a `*/` (anywhere!). For example:
```mojom
/* This is a
   multi-line comment. */

/* /* Comments don't nest. */

/* // The "//" is meaningless here. */

/* "This isn't a string literal. */

interface/*This_separates_tokens*/AnInterface {};

const string kAConstString = "/* This is not a comment. */";
```

### File structure

Apart from comments and whitespace, a Mojom file consists of, in order:
* an optional *module* declaration;
* zero or more *import* statements (the order of these is not important); and
* zero or more declarations of *struct*s, *interface*s, *union*s, *enum*s, or
  *const*s (the order of these is not important).
These elements will be described in following sections.

As stated above, the order of struct/interface/union/enum/const declarations is
not important. This is required to allow "cyclic" structures to be defined.
Nonetheless, whenever possible, one should declare things before they are
"used". For example, the following is valid but not recommended:
```mojom
// NOT recommended.

const MyEnum kMyConst = kMyOtherConst;
const MyEnum kMyOtherConst = A_VALUE;

enum MyEnum {
  A_VALUE,
  ANOTHER_VALUE,
};
```

## Names and identifiers

*Names* in Mojom start with a letter (`a`-`z`, `A`-`Z`) and are followed by any
number of letters, digits (`0`-`9`), or underscores (`_`). For example:
`MyThing`, `MyThing123`, `MyThing_123`, `my_thing`, `myThing`, `MY_THING`. (See
below for naming conventions, however.)

Various things in Mojom are named (i.e., assigned names):
* types (e.g., interfaces, structs, unions, and enums),
* things associated with particular types (e.g., messages in interfaces,
  parameters in messages, fields in structs and unions, and values in enums),
  and
* const values.

*Identifiers* consist of one or more names, separated by `.`. These are used in
module declarations, as well as in referencing other named things.

### Namespacing and name resolution

As mentioned above, not only are types named, but things associated with a given
type may be named. For example, consider:
```mojom
enum MyEnum {
  A_VALUE,
  ANOTHER_VALUE,
  A_DUPLICATE_VALUE = A_VALUE,
};
```
`MyEnum` is the name of an enum type, and `A_VALUE` is the name of a value of
`MyEnum`. Within the *scope* of `MyEnum` (or where that scope is implied),
`A_VALUE` may be used without additional qualification. Outside that scope, it
may be referred to using the identifier `MyEnum.A_VALUE`.

Some type definitions allow (some) other type definitions to be nested within.
For example:
```mojom
struct MyStruct {
  enum MyEnum {
    A_VALUE,
  };

  MyEnum my_field1 = A_VALUE;
  MyStruct.MyEnum my_field2 = MyStruct.MyEnum.A_VALUE;
};
```
Within `MyStruct`, `MyEnum` may be referred to without qualification (e.g., to
define the field `my_field1`). Outside, it may be referred to using the
identifier `MyStruct.MyEnum`. Notice that `my_field1` is assigned a default
value of `A_VALUE`, which is resolved correctly since there is an implied scope
of `MyEnum`. It would also be legal to write the default value as
`MyEnum.A_VALUE` or even `MyStruct.MyEnum.A_VALUE`, as is done for `my_field2`.

Thus names live in a name hierarchy, with the "enclosing" scopes often being
other type names. Additionally, *module names* (see below) can be used to define
artificial outermost scopes.

Names (or, more properly, identifiers) are resolved in a C++-like way: Scopes
are searched from inside outwards, i.e., starting with the current, innermost
scope and then working outwards.

### Standard naming style

Though Mojom allows arbitrary names, as indicated above, there are standard
stylistic conventions for naming different things. Code generators for different
languages typically expect these styles to be followed, since they will often
convert the standard style to one appropriate for their target language. Thus
following the standard style is highly recommended.

The standard styles are (getting ahead of ourselves slightly):
* `StudlyCaps` (i.e., concatenated capitalized words), used for user-defined
  (struct, interface, union, enum) type names and message (a.k.a. function or
  method) names;
* `unix_hacker_style` (i.e., lowercase words joined by underscores), used for
  field (a.k.a. "parameter" for messages) names in structs, unions, and
  messages;
* `ALL_CAPS_UNIX_HACKER_STYLE` (i.e., uppercase words joined by underscores),
  used for enum value names; and
* `kStudlyCaps` (i.e., `k` followed by concatenated capitalized words), used for
  const names.

## Module statements

The Mojom *module* statement is a way of logically grouping Mojom declarations.
For example:
```mojom
module my_module;
```
Mojom modules are similar to C++ namespaces (and the standard C++ code generator
would put generated code into the `my_module` namespace), in that there is no
implication that the file contains the entirety of the "module" definition;
multiple files may have the same module statement. (There is also no requirement
that the module name have anything to do with the file path containing the Mojom
file.)

The specified Mojom module name is an identifier: it can consist of multiple
parts separated by `.`. For example:
```mojom
module my_module.my_submodule;

struct MyStruct {
};
```
Recall that name look-up is similar to C++: E.g., if the current module is
`my_module.my_submodule` then `MyStruct`, `my_submodule.MyStruct`, and
`my_module.my_submodule.MyStruct` all refer to the above struct, whereas if the
current module is just `my_module` then only the latter two do.

Note that "module name" is really a misnomer, since Mojom does not actually
define modules per se. Instead, as suggested above, module names play only a
namespacing role, defining the "root" namespace for the contents of a file.

## Import statements

An *import* statement makes the declarations from another Mojom file available
in the current Mojom file. Moreover, it operates transitively, in that it also
makes the imports of the imported file available, etc. The "argument" to the
import statement is a string literal that is interpreted as the path to the file
to import. Tools that work with Mojom files are typically provided with a search
path for importing files (just as a C++ compiler can be provided with an
"include path"), for the purposes of resolving these paths. (**TODO(vtl)**: This
always includes the current Mojom file's path, right? Is the current path the
first path that's searched?)

For example:
```mojom
module my_module;

import "path/to/another.mojom";
import "path/to/yet/a/different.mojom";
```
This makes the contents of the two specified Mojom files available, together
with whatever they import, transitively. (Names are resolved in the way
described in the previous section.)

Import cycles are not permitted (so, e.g., it would be an error if
`path/to/another.mojom` imported the current Mojom file). However, it is
entirely valid for Mojom files to be imported (transitively) multiple times
(e.g., it is fine for `path/to/another.mojom` to also import
`path/to/yet/a/different.mojom`).

## Types in Mojom

*Types* in Mojom are really only used in two ways:
* first, in declaring additional types (recall that the Mojom language is nearly
  entirely about defining types!); and
* second, in declaring const values.
The first way really covers a lot of ground, however. Type identifiers (i.e.,
identifiers that resolve to some type definition) may occur in:
* field declarations within struct and union declarations;
* in message declarations (in both request and response parameters) in interface
  declarations (this is really very similar to the use in struct field
  declarations); and
* in "composite" type specifiers (e.g., to specify an array of a given type).

### Reference and non-reference types

There are two basic classes of types, *reference* types and *non-reference*
types. The latter class is easier to understand, consisting of the built-in
number (integer and floating-point) types, as well as user-defined enum types.
All other types are reference types: they have some notion of *null* (i.e.,
non-presence).

#### Nullability

When an identifier is used (in another type definition, including in message
parameters) to refer to a reference type, by default the instance of the type is
taken to be *non-nullable*, i.e., required to not be null. One may allow that
instance to be null (i.e., specify a *nullable* instance) by appending `?` to
the identifier. For example, if `Foo` is a reference type:
```mojom
struct MyStruct {
  Foo foo1;
  Foo? foo2;
};
```
In an instance of `MyStruct`, the `foo1` field may never be null while the
`foo2` field may be null.

This also applies to composite type specifiers. For example:
* `array<Foo>` is a non-nullable array of non-nullable `Foo` (the array itself
  may not be null, nor can any element of the array);
* `array<Foo?>` is a non-nullable array of nullable `Foo` (the array itself may
  not be null, but any element of the array may be null);
* `array<Foo>?` is a nullable array of non-nullable `Foo`; and
* `array<Foo?>?` is a nullable array of nullable `Foo`.
(See below for details on arrays.)

### Built-in types

#### Simple types

#### Strings

#### Arrays

#### Maps

#### Raw handle types


**TODO(vtl)**: The stuff below is old stuff to be reorganized/rewritten.

## Interfaces

A Mojom *interface* is (typically) used to describe communication on a message
pipe. Typically, message pipes are created with a particular interface in mind,
with one endpoint designated the *client* (which sends *request* messages and
receives *response* messages) and the other designed that *server* or *impl*
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
We will discuss in greater detail how structs are declared later.

### Names in Mojom

Names in Mojom are not important. Except in affecting compatibility at the level
of source code (when generating bindings), names in a Mojom file may be changed
arbitrarily without any effect on the "meaning" of the Mojom file (subject to
basic language requirements, e.g., avoiding collisions with keywords and other
names). E.g., the following is completely equivalent to the interface discussed
above:
```mojom
interface Something {
  One(int32 an_integer, string a_string);
  Two() => (bool a_boolean, uint32 an_unsigned);
  Three() => ();
};
```
The `Something` interface is compatible at a binary level with `MyInterface`. A
client using the `Something` interface may communicate with a server
implementing the `MyInterface` with no issues, and vice versa.

The reason for this is that elements (messages, parameters, struct members,
etc.) are actually identified by *ordinal* value. They may be specified
explicitly (using `@123` notation; see below). If they are not specified
explicitly, they are automatically assigned. (The ordinal values for each
interface/struct/etc. must assign distinct values for each item, in a
consecutive range starting at 0.)

Explicitly assigning ordinals allows Mojom files to be rearranged "physically"
without changing their meaning. E.g., perhaps one would write:
```mojom
interface MyInterface {
  Bar@1() => (bool x@0, uint32 y@1);
  Baz@2() => ();

  // Please don't use this in new code!
  FooDeprecated@0(int32 a@0, string b@1);
};
```

Ordinals also tie into the versioning scheme (**TODO(vtl)**: link?), which
allows Mojom files to be evolved in a backwards-compatible way. We will not
discuss this matter further here.

**TODO(vtl)**: Maybe mention exceptions to this in attributes (e.g.,
`ServiceName`).

### Struct declarations

A Mojom *struct* declaration consists of a finite sequence of *field
declaration*, each of which consists of a *type*, a *name*, and optionally a
*default value* (if applicable for the given type). (If no default value is
declared, then the default is the default value for the field type, typically 0,
null, or similar.)

Additionally, a struct may contain enum and const declarations (**TODO(vtl)**:
why not struct/union/interface declarations?). While the order of the field
declarations (with respect to one another) is important, the ordering of the
enum/const declarations (with respect to both the field declarations and other
enum/const declarations) is not. (But as before, we recommend declaring things
before "use".)

Here is an example with these elements:
```mojom
struct Foo {
  const int8 kSomeConstant = 123;

  enum MyEnum {
    A_VALUE,
    ANOTHER_VALUE
  };

  int8 first_field = kSomeConstant;
  uint32 second_field = 123;
  MyEnum etc_etc = A_VALUE;
  float a;    // Default value is 0.
  string? b;  // Default value is null.
};
```
(Note that `kSomeConstant` may be referred to as `Foo.kSomeConstant` and,
similarly, `MyEnum` as `Foo.MyEnum`. This is required outside of the `Foo`
declaration.)

### Interface declarations

**TODO(vtl)**

### Union declarations

**TODO(vtl)**

### Enum declarations

**TODO(vtl)**

### Const declarations

**TODO(vtl)**

**TODO(vtl)**: Write/(re)organize the sections below.

## Data types

### Primitive types

#### Standard types

#### Enum types

### "Pointer" types

#### Nullability

#### Strings

#### Maps

#### Structs

#### Arrays

#### Unions

### Handle types

#### Raw handle types

#### Interface types

#### Interface request types

## Annotations

## Pipelining
