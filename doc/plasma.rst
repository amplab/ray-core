Plasma: Storing objects in memory
=================================

Plasma is a shared region of memory that allows multiple processes running on
the same machine to access shared data objects.

An object is created in two distinct phases:

1. Allocate memory and write data into allocated memory.
   If the size of the data is not known in advance, the buffer can be resized.
   Note that during this phase the buffer is writable, but only by its
   creator. No one else can access the buffer during this phase.

2. Seal the buffer. Once the creator finishes writing data into buffer
   it seals the buffer. From this moment on the buffer becomes
   immutable and other processes can read it.

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

Attaching a client to Plasma
----------------------------

.. doxygenclass:: plasma::ClientContext
   :project: ray
   :members:

Plasma metadata
---------------

Each object has a small key value store associated which is used to store
metadata. Users can provide arbitrary information here.
