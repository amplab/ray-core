# Message pipes

Message pipes are the primary communication mechanism between Mojo programs. A
*message pipe* is a point-to-point (with each side being called a message pipe
*endpoint*), bidirectional message passing mechanism, where messages may contain
both data and Mojo handles.

It's important to understand that a message pipe is a "transport": it provides a
way for data and handles to be sent between Mojo programs. The system is unaware
of the meaning of the data or of the handles (other than their intrinsic
properties).

That said, Mojo provides a *standard* way of communicating over message pipes,
namely via a standardized protocol together with [Mojom IDL](mojom_idl.md)
files.

## Messages

A *message* consists of two things:
* a finite sequence of bytes, and
* a finite sequence of [Mojo handles](handles.md).

Both of these are determined when the message is sent (or *written*). Messages
are *framed* in the sense that they are "atomic" units: they are sent and
received (or *read*) as entire units, not just by Mojo programs themselves but
by the system, which is aware of and enforces the message boundaries.

(Note on terminology: We'll use "send" and "write" interchangeably, and
similarly for "receive" and "read". "Write" and "read" correspond more closely
to the names usually given to the basic Mojo operations, e.g.,
`MojoWriteMessage()` and `MojoReadMessage()`.)

## Asynchronous operation and queueing

Message pipes are *asynchronous* in the sense that sent messages do not have
intrinsic response messages mediated/enforced by the system. (This is different
from saying that message write/read are asynchronous operations: these
operations are actually synchronous and complete "immediately". However, note
that reading a message is "nonblocking" in the sense that it will fail if a
message is not available to be read. Thus a message must be waited for, and the
combined wait-then-read may form an asynchronous pattern.)

To allow message writes to complete immediately, messages are queued. Indeed,
one can understand a message pipe as a pair of queues, one in each direction.
Each endpoint has opposite notions of incoming and outgoing queues (recall that
message pipes have a pair of endpoints).

Writing a message to an endpoint then simply entails enqueueing that message on
that endpoint's outgoing queue (which is the *peer* endpoint's incoming queue).
Reading a message from an endpoint is just dequeueing a message from that
endpoint's incoming queue (which is the peer endpoint's outgoing queue).

Queueing is unlimited. Why? The problem is that limiting queueing exposes Mojo
programs to complex deadlock problems:
* One way of limiting queue sizes is to block the sender if the queue is "full".
  However, the receiver may not be able or willing to consume messages until the
  sender does something else (and this is often the case in asynchronous
  programming). For example, perhaps the putative "receiver" does not yet even
  have a handle to the endpoint yet, and that handle is sent in a message (over
  some other message pipe).
* Another way would be to have the write fail if the queue is full. Then the
  sender would want to additionally queue on its side. The thread would continue
  running and, e.g., run its message loop. However, sender-side queueing
  basically makes it impossible for the sender to transfer that endpoint
  (handle), at least until the sender-side queue is cleared. However, the
  receiver may not be able/willing to proceed until the sender has transferred
  the aforementioned endpoint.

Thus we allow unlimited queueing. (**TODO(vtl)**: Probably we'll eventually
allow "hard" queue limits to be set for the purposes of preventing
denial-of-service. However, the response to overruns will be hard failures, in
the sense that the message pipe may be destroyed, rather than soft,
"recoverable" failures -- since those expose deadlock issues.) It is then up to
Mojo programs to implement flow control in some way. (**TODO(vtl)**: Write more
about this in the context of Mojom.)

## "Synchronous" operation

Though message pipes are asynchronous as discussed above, they may be used in a
synchronous fashion: immediately after sending a *request* message, one can then
just block and wait for the *response* message (and then read it and process
it). Of course, this requires that the protocol support this:
* Message pipes must be used in a "directional" way: there must be fixed request
  and response directions or, equivalently, one endpoint belongs to the *client*
  and the other to the *server* (or *impl*). (Historical note: This is the case
  for the current [Mojom protocol](mojom_protocol.md), but not in previous
  versions.) The issue here is that without this, the sender of the request
  messages may have to process incoming request messages from its peer.
* Request messages must have unique response messages. (In the Mojom protocol,
  request messages have optional unique responses. For messages without
  responses, one can just proceed immediately without waiting. However, without
  response messages there may be flow control issues; again, see above.) The
  important point is that for each request message, there is a well-defined
  number of response messages for each request and not arbitrary "callback"
  messages.
* The sending of a response message must not depend on a future action of the
  client. (This is a higher-level semantic that is not enforced by the Mojom
  protocol. E.g., one may define a Mojom interface in which the response to a
  message *Foo* isn't sent until the client sends a request *Bar*.)

That said, whether one wants to, or even can, use this synchronous mode of
operation may depend on a number of things:
* For message-loop-centric programming languages, this mode is at best
  undesirable (and possibly infeasible, depending on what facilities are exposed
  to user code). (E.g., this is the case for JavaScript and Dart.)
* Similarly, on a message-loop programming model (in which threads -- if there
  are more than one -- are mostly coordinated by "message passing"), it is
  typically undesirable to block any thread (with a message loop). Indeed, if
  other message pipes are serviced by a message loop, blocking the thread may
  result in deadlock. (E.g., this is the usual programming model for the
  standard Mojom C++ bindings.)
* Even when blocking is permissible, it may not be desirable to do so:
  advancement of the program then relies on trusting the server to be
  responsive and send responses in a timely fashion.
* Mixing asynchronous and synchronous operation is problematic: one cannot send
  a request and synchronously wait for a response while responses to other
  messages are still pending. (Theoretically, one could buffer such other
  responses until the response to particular request is received, and process
  those other responses later, but this would be dubious at best.)
