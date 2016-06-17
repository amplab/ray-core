About
=====

This directory contains the services required to execute both nexes and pexes.

Using
=====

For information about how to build and use nexes and pexes from within Mojo,
refer to the `mojo/nacl` directory.

Non-SFI Nexe Content Handler
----------------------------

The nexe content handler is simple. It:

1. Acquires the desired nexe and dumps it into a temporary file,
2. Accesses the MojoHandle connecting the content handler to the shell, and
3. Launches the Non-SFI nexe.

Non-SFI Pexe Content Handler
----------------------------

The pexe content handler is slightly more complex. Although it is similar
in structure to the Non-SFI nexe content handler, it has an additional step
between item 1 and 2: convert the incoming pexe into a nexe.

This pexe to nexe translation requires two steps: compilation and linking.
For each of these steps, a helper service is launched. These helper services
are actually executed as nexes -- `pnacl_llc.nexe` and `ld.nexe`. The
translation done by these nexes is executed as part of a callback to IRT
functions, `nacl_irt_private_pnacl_translator_compile`, and
`nacl_irt_private_pnacl_translator_link`. This makes communication between
the content handler and these helper nexes more complicated.

For the full picture of the compilation process:

* PexeContentHandler
  * Hash the input file, and observe if it has already been translated.
    If it has, then read the pre-translated nexe from storage and launch it
    immediately.
  * Create a message pipe, which has a parent and child handle.
  * Call `PexeCompilerStart`, passing in the child end of the message pipe. This
  contacts a new service which is responsible for launching `pnacl_llc.nexe`.
* PexeCompiler (new process)
  * Launch the pnacl_llc nexe using the child end of the pipe received from the
  PexeContentHandler
  * The pnacl_llc nexe uses the IRT to make a call to ServeTranslateRequest
  (defined in `mojo/nacl/nonsfi/irt_pnacl_translator_compile.cc`). This creates
  the `PexeCompiler` service, which is ready to handle a single request. It is
  bound to the child end of the pipe.
* Meanwhile, back in the PexeContentHandler:
  * Bind the parent end of the pipe to a `PexeCompiler` service. Now,
  `PexeCompile` can be called with inputs defined by a mojom interface, and
  outputs can be received via callback
  * Call `PexeCompile`, passing in the name of the pexe, and receiving the
  object files created by compilation
* Meanwhile, back in the pnacl_llc nexe:
  * Actually do the requested compilation, and pass back the newly created
  object files.

The linking process works similarly, but utilizes a different interface which
lets it receive object files and return a linked nexe. Once linking has
finished, the PexeContentHandler may choose to cache the resultant nexe so that
future clients accessing the same pexe will be able to skip the translation
process.

Once both the compilation and linking steps have been completed, the
PexeContentHandler is able to launch the requested nexe.

Note: For x86 and x86-64 systems, Subzero is used for translating the pexe
into a native format, and `sz.nexe` is used instead of `pnacl-llc.nexe`.
