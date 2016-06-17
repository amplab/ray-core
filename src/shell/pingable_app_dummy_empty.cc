// This file contains no code but must exist so the
// static_library("pingable_app_dummy") target has a parameter. This target has
// to be a static_library, not a source_set, to hide linker dependencies (see
// the BUILD.gn file for more details). This file must exist as some linkers
// when told to produce a static library generate an error if passed an empty
// set of files.
