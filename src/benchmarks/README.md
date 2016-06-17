# benchmarks

This directory contains mojo applications that are run as performance benchmarks
by [mojo_benchmark](../mojo/devtools/common/mojo_benchmark).

See also:
 - [documentation](../mojo/devtools/common/docs/mojo_benchmark.md) for
   `mojo_benchmark`

## Legacy benchmarks
`benchmark_runner.py` and the `startup` benchmark are the remainings of the
previous take on performance benchmarks. These can be dropped once we can track
startup time of the shell itself using the new system.
