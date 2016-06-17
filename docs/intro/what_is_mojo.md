# What is Mojo?

Mojo is a layered system for programs to interact with the "system". The system
should be compared to a microkernel-based operating system. That is, most system
services are provided via IPC (interprocess communication), especially via
message passing, with other Mojo programs.

Broadly speaking, there are the following layers to Mojo:
* The Mojo system API: low-level API for basic program operation and IPC.
* Mojom: a protocol for communicating over IPC (together with a language, also
  called Mojom, for describing messages and data).
* Descriptions of services provided via IPC (usually specified using the Mojom
  language, together with additional human-readable text to specify additional
  semantics, etc.).
* Implementations of those services.

## The Mojo system API

At the lowest layer, Mojo provides a "raw" API for interacting with the system,
which should be fairly minimal due to the microkernel-like nature of the system.

Mojo is intended to be language-agnostic. It should be possible to write
programs in many different languages, which interact with other Mojo programs
(possibly written in different languages). As such there is no single Mojo
system API: some things that may be required for one language may simply be
inappropriate for another. (Indeed, there may even be multiple versions of "the"
Mojo system API even for a single language.)

Due to the nature of the system, there is a common thread among the Mojo system
APIs provided to different languages, namely IPC. It is desirable to provide
low-level access to IPC (e.g., sending/receiving messages at the byte level), so
that programs written in a given programming language have "first-class" access
to the basic communication primitives.

All that said, at the lowest level, the Mojo system API for native programs is
intended to be complete, in the sense that it is the only API that is used to
interact with the operating system (defining this API is a work in progress).
Thus it must include basic mechanisms for memory management, thread
creation/destruction, synchronization, etc.

## Mojom

As hinted at above, "Mojom" itself consists of several layers:
* The lowest, most essential layer is a protocol (or family of protocols),
  consisting of semantic specifications and byte-level message formats.
  (This layer is the only one that is essential for interoperability between
  programs.)
* On a given *message pipe*, certain messages may be sent (in one direction or
  another). Mojom includes a language for specifying those messages (and also
  relatedly includes a way of specifying data formats), grouped into
  *interfaces*. (Thus the Mojom language is often referred to as an *interface
  description language* or *IDL*.) An interface specified in the Mojom language
  can then be interpreted to provide a description of messages at the byte level
  (and related low-level semantics, e.g., which messages require *response*
  messages).
* A tool for interpreting Mojom files: this tool takes as input files written in
  the Mojom language. For supported programming languages, it then generates
  code to make using or implementing interfaces in that language easier.
  (Provided with low-level access to IPC, one can of course always, e.g., send
  the correct sequence of bytes, but this would hardly be practical.)

**TODO(vtl)**

## Service descriptions

**TODO(vtl)**

## Service implementations

**TODO(vtl)**
