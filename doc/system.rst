The Ray shell
==========================

The Ray shell is responsible for managing all the services that are running
on a given node, like the local scheduler, the Plasma store and the Python
workers. There is one shell per node.

You can start the shell using

::

  ./mojo_shell --enable-multiprocess
               --external-connection-address=/home/ubuntu/shell
               ray_node_app.mojo

This starts ray_node_app.mojo, which starts the object store and listens on
the socket ``/home/ubuntu/shell`` to establish connections to Python and C++
clients.
