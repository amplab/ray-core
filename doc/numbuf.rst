Numbuf: Serializing Python data
===============================

Numbuf is a library for serialization of nested Python data objects
to the `Apache Arrow <https://arrow.apache.org/>`_ format.

Datatypes that can be serialized at the moment (see numbuf/python/test/runtest.py):

- Python scalars: int, long, float, strings, None, bool
- Python collections: lists, tuples, dicts
- Numpy ndarrays (int8, int16, int32, int64, unsigned versions of these, float32, float64)
- Numpy scalars, same types as the above
- Arbitrary nestings of the above

Note that Arrow is designed for serialization of collections of objects. If the
value you are serializing is not a Python list, you should wrap it into a
Python singleton list.

Serializing a Python object typically happens in three steps:

  1. An Arrow `RowBatch <https://github.com/apache/arrow/blob/master/cpp/src/arrow/table.h>`_
     is constructed from the object using `serialize_list`.
     During this phase, we compute the size that the serialized object will
     occupy on disk (in bytes) as well as the schema of the object. These
     are returned and the (internal) buffers containing the parts of the object
     are assembled.
  2. A buffer of the appropriate size is allocated by the application logic.
  3. The object is written into the buffer using `write_to_buffer`.

The schema is stored in the `Arrow metadata format
<https://github.com/apache/arrow/blob/master/format/Message.fbs>`_
and can be manipulated and viewed using Arrow tools.

Deserialization works like this:

  1. The RowBatch is read from the buffer using `read_from_buffer`. A schema
     that describes the data needs to be specified as a bytearray.

  2. The Python object is deserialized from the RowBatch using `deserialize_list`.

Example
-------

Here is an example on how you can use the library to serialize and deserialize
a simple Python value into a numpy array. Not that you can serialize data into
any Python memoryview.

.. code-block:: python
   :linenos:

   import numpy as np
   import libnumbuf

   # Define an object
   obj = [1, 2, 3, "hello, world"]

   # Serialize the object into a numpy array
   schema, size, batch = libnumbuf.serialize_list(obj)
   buff = np.zeros(size, dtype="uint8")
   libnumbuf.write_to_buffer(batch, memoryview(buff))

   # Deserialize the object from the numpy array
   array = libnumbuf.read_from_buffer(memoryview(buff), schema)
   result = libnumbuf.deserialize_list(array)

   # Assert the deserialized object agrees with the original object
   assert obj == result

The API
--------------

.. py:function:: serialize_list(value)

   Serialize a Python list into an Arrow RowBatch

   :param list value: The Python list to be serialized
   :return: A Python tuple containing:

     - A Python bytearray with the schema metadata
     - The size in bytes the serialized object will occupy in memory
     - A Python capsule wrapping the Arrow RowBatch

   :raises numbuf.error: If the value contains non-serializable objects

.. py:function:: deserialize_list(batch)

   Deserialize an Arrow RowBatch into a Python list

   :param PyCapsule batch: A Python capsule wrapping the Arrow RowBatch
   :return: The Python list represented by the RowBatch

.. py:function:: write_to_buffer(batch)

   Write an Arrow RowBatch into a memory buffer

   :param PyCapsule batch: A Python capsule wrapping the Arrow RowBatch
   :param memoryview buffer: A writable Python buffer

.. py:function:: read_from_buffer(buffer, schema)

   Read serialized data from a readable buffer into an Arrow RowBatch

   :param memoryview buffer: A readable Python buffer that contains the data
   :param bytearray schema: The schema metadata
   :return: A Python capsule wrapping the Arrow RowBatch
