Developer documentation for Ray
===============================

Client connections to the Shell
-------------------------------

Most of the complexity of this code comes from the fact that we need to
be able to connect to the Ray shell from an outside process (i.e. a Python process)
that was started independently of the Ray shell. This is not supported in
Mojo, they use fork to start child processes.

.. doxygenclass:: ray::FileDescriptorSender
   :project: ray
   :members:

.. doxygenclass:: ray::FileDescriptorReceiver
   :project: ray
   :members:

.. doxygenclass:: shell::ServiceConnectionApp
   :project: ray
   :members:
