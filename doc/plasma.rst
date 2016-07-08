Plasma: Storing objects in memory
=================================

Plasma is a shared region of memory that allows multiple processes running on
the same machine to access shared data objects.

It can be used both as a Ray service and a library in your own programs.

An object is created in two distinct phases:

1. Allocate memory and write data into allocated memory.
   If the size of the data is not known in advance, the buffer can be resized.
   Note that during this phase the buffer is writable, but only by its
   creator. No one else can access the buffer during this phase.

2. Seal the buffer. Once the creator finishes writing data into buffer
   it seals the buffer. From this moment on the buffer becomes
   immutable and other processes can read it.

To create an object, the user specifies a unique identifier for the object and
an optional name. Plasma keeps track of the process id that created the object,
the creation time stamp, how long creation of the object took and the size of
the object. During creation, the user can also specify metadata that will be
associated with the object.

Other processes can request an object by its unique identifier (later also by
name). If the object has not been created or sealed yet, the process requesting
the object will block until the object has been sealed.

The Buffer interface
--------------------

A buffer is the region of memory associated to a data object, as determined by a
start address and a size in bytes. There are two kinds of buffers, read-only
buffers and read-write buffers.

.. doxygenclass:: plasma::Buffer
   :project: ray
   :members:

MutableBuffers have a richer interface, they allow writing to and resizing
the object. When the object creator has finished modifying the object, it
calls the Seal method to make the object immutable, which allows other
processes to read the object.

.. doxygenclass:: plasma::MutableBuffer
   :project: ray
   :members:

The Plasma client interface
---------------------------

The developer interacts with Plasma through the Plasma API. Each process
needs to instantiate a ClientContext, which will give the process access to
objects and their metadata and allow them to write objects.

.. doxygenclass:: plasma::ClientContext
   :project: ray
   :members:

Plasma metadata
---------------

There are two parts to the object metadata: One internally maintained by Plasma
an one provided by the user. The first part is represented by the ObjectInfo class.

.. doxygenclass:: plasma::ObjectInfo
   :project: ray
   :members:

Each object has a small dictionary that can hold metadata provided by users.
Users can provide arbitrary information here. It is most likely going to be
used to store information like ``format`` (``binary``, ``arrow``, ``protobuf``,
``json``) and ``schema``, which could hold a schema for the data.

An example application
----------------------

We are going to have more examples here. Currently, the best way of
understanding the API is by looking at ``libplasma``, the Python C extension
for Plasma. It can be found in https://github.com/amplab/ray-core/blob/master/src/plasma/client/plasma.cc.

Note that this is not the Python API that users will interact with.
It can be used like this:

::

  import libplasma

  plasma = libplasma.connect("/home/pcmoritz/shell")

  A = libplasma.build_object(plasma, 1, 1000, "object-1")
  libplasma.seal_object(A)
  B = libplasma.build_object(plasma, 2, 2000, "object-2")
  libplasma.seal_object(B)

  libplasma.list_objects(plasma)
