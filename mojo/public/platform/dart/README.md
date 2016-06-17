Dart Embedding Support
======================

This directory contains the implementation of the native methods used by
the 'dart:mojo.internal' library.

NOTE: The contents of this directory should only be used if you are implementing
a custom content handler that runs dart code that expects 'dart:mojo.internal'.

NOTE: The build rules are written in such a way that the Dart SDK is checked out
under //dart.

NOTE: The sources in this directory indirectly coupled to the revision of the
Dart SDK checked out under //dart. You must ensure that you are using the
same revision.