ForwardingContentHandler
=====================

This content handler forwards to another "target" Mojo application.
The content handler reads the target application's URL
from the initial URL and then connects to it.

If you change the sample.fch file in this directory so that it
contains a legitimate Mojo application URL, then to run this
appllication with mojo_shell, set DIR to be the absolute
path for this directory, then:

  mojo_shell "file://$DIR/sample.fch"





