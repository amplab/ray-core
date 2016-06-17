# Mojo handles (objects)

Mojo handles are analogous to Unix file descriptors or Windows `HANDLE`s. That
is, they are basically-opaque integers that a program can use to refer to system
resources. Like their Unix and Windows equivalents, a Mojo handle value only has
meaning within a given process.

That said, there are some differences:
* There is a single value, 0, that is guaranteed to never be a valid Mojo
  handle. This is unlike Unix, where all negative file descriptors are usually
  taken to be invalid, even if -1 is often taken to be the "canonical" invalid
  file descriptor value.
* The allocation of Mojo handle values is not specified. This is unlike Unix,
  where file descriptors are allocated sequentially, with the lowest (positive)
  unused value allocated.
* Unlike Windows, there are no "pseudohandles". That is, there is no Mojo handle
  value whose meaning is context dependent (within the same process).
* In general, Mojo handles need not be duplicatable, whereas their Unix and
  Windows equivalents can universally be duplicated.
* Mojo handles can be sent across [message pipes](message_pipes.md). Unlike
  sending file descriptors over Unix domain sockets (using `SCM_RIGHTS`), this
  is done with transfer semantics: after a message with attached Mojo handles is
  sent, the Mojo handle values become invalid in the sending process. (Even if
  the receiving process is the same as the sending process, the received Mojo
  handle values will probably be different from the values that were sent.)
* Each Mojo handle has a set of "rights", which control what operations can be
  performed on a given handle. The rights for a given handle are immutable.
  However, any handle may be replaced with an "equivalent" handle with a
  (possibly) reduced set of rights.
* A Mojo handle has a well-defined life-cycle, and is only invalidated either by
  being transferred across a message pipe, by being replaced (by an "equivalent"
  handle), or by being closed. Unlike Unix's overloaded `close()` (which may
  fail due to data loss, i.e., inability to flush), closing a (valid) Mojo
  handle never fails.
