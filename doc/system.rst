Components of the System
==========================

The Ray shell
-------------

There is one Ray shell running on each node. It is responsible for starting
all the services required on that node (like the scheduler, Plasma, the worker
processes, etc.).
