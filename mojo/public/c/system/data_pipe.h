// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains types/constants and functions specific to data pipes.
//
// Note: This header should be compilable as C.

#ifndef MOJO_PUBLIC_C_SYSTEM_DATA_PIPE_H_
#define MOJO_PUBLIC_C_SYSTEM_DATA_PIPE_H_

#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/macros.h"
#include "mojo/public/c/system/result.h"

// |MojoCreateDataPipeOptions|: Used to specify creation parameters for a data
// pipe to |MojoCreateDataPipe()|.
//   |uint32_t struct_size|: Set to the size of the |MojoCreateDataPipeOptions|
//       struct. (Used to allow for future extensions.)
//   |MojoCreateDataPipeOptionsFlags flags|: Used to specify different modes of
//       operation.
//     |MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE|: No flags; default mode.
//   |uint32_t element_num_bytes|: The size of an element, in bytes. All
//       transactions and buffers will consist of an integral number of
//       elements. Must be nonzero.
//   |uint32_t capacity_num_bytes|: The capacity of the data pipe, in number of
//       bytes; must be a multiple of |element_num_bytes|. The data pipe will
//       always be able to queue AT LEAST this much data. Set to zero to opt for
//       a system-dependent automatically-calculated capacity (which will always
//       be at least one element).

typedef uint32_t MojoCreateDataPipeOptionsFlags;

#define MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE \
  ((MojoCreateDataPipeOptionsFlags)0)

MOJO_STATIC_ASSERT(MOJO_ALIGNOF(int64_t) == 8, "int64_t has weird alignment");
struct MOJO_ALIGNAS(8) MojoCreateDataPipeOptions {
  uint32_t struct_size;
  MojoCreateDataPipeOptionsFlags flags;
  uint32_t element_num_bytes;
  uint32_t capacity_num_bytes;
};
MOJO_STATIC_ASSERT(sizeof(struct MojoCreateDataPipeOptions) == 16,
                   "MojoCreateDataPipeOptions has wrong size");

// |MojoDataPipeProducerOptions|: Used to specify data pipe producer options (to
// |MojoSetDataPipeProducerOptions()| and from
// |MojoGetDataPipeProducerOptions()|).
//   |uint32_t struct_size|: Set to the size of the
//       |MojoDataPipeProducerOptions| struct. (Used to allow for future
//       extensions.)
//   |uint32_t write_threshold_num_bytes|: Set to the minimum amount of space
//       required to be available, in number of bytes, for the handle to be
//       signaled with |MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD|; must be a multiple
//       of the data pipe's element size. Set to zero to opt for the default,
//       which is the size of a single element.

struct MOJO_ALIGNAS(8) MojoDataPipeProducerOptions {
  uint32_t struct_size;
  uint32_t write_threshold_num_bytes;
};
MOJO_STATIC_ASSERT(sizeof(struct MojoDataPipeProducerOptions) == 8,
                   "MojoDataPipeProducerOptions has wrong size");

// |MojoWriteDataFlags|: Used to specify different modes to |MojoWriteData()|
// and |MojoBeginWriteData()|.
//   |MOJO_WRITE_DATA_FLAG_NONE| - No flags; default mode.
//   |MOJO_WRITE_DATA_FLAG_ALL_OR_NONE| - Write either all the elements
//       requested or none of them.

typedef uint32_t MojoWriteDataFlags;

#define MOJO_WRITE_DATA_FLAG_NONE ((MojoWriteDataFlags)0)
#define MOJO_WRITE_DATA_FLAG_ALL_OR_NONE ((MojoWriteDataFlags)1 << 0)

// |MojoDataPipeConsumerOptions|: Used to specify data pipe consumer options (to
// |MojoSetDataPipeConsumerOptions()| and from
// |MojoGetDataPipeConsumerOptions()|).
//   |uint32_t struct_size|: Set to the size of the
//       |MojoDataPipeConsumerOptions| struct. (Used to allow for future
//       extensions.)
//   |uint32_t read_threshold_num_bytes|: Set to the minimum number of bytes
//       required to be available for the handle to be signaled with
//       |MOJO_HANDLE_SIGNAL_READ_THRESHOLD|; must be a multiple
//       of the data pipe's element size. Set to zero to opt for the default,
//       which is the size of a single element. Note: If the producer handle is
//       closed with less than this amount of data in the data pipe, the
//       consumer's |MOJO_HANDLE_SIGNAL_READ_THRESHOLD| will be considered
//       unsatisfiable; if there is actually some data remaining in the data
//       pipe, the status of this signal may be changed if this value is
//       modified.

struct MOJO_ALIGNAS(8) MojoDataPipeConsumerOptions {
  uint32_t struct_size;
  uint32_t read_threshold_num_bytes;
};
MOJO_STATIC_ASSERT(sizeof(struct MojoDataPipeConsumerOptions) == 8,
                   "MojoDataPipeConsumerOptions has wrong size");

// |MojoReadDataFlags|: Used to specify different modes to |MojoReadData()| and
// |MojoBeginReadData()|.
//   |MOJO_READ_DATA_FLAG_NONE| - No flags; default mode.
//   |MOJO_READ_DATA_FLAG_ALL_OR_NONE| - Read (or discard) either the requested
//        number of elements or none.
//   |MOJO_READ_DATA_FLAG_DISCARD| - Discard (up to) the requested number of
//        elements.
//   |MOJO_READ_DATA_FLAG_QUERY| - Query the number of elements available to
//       read. For use with |MojoReadData()| only. Mutually exclusive with
//       |MOJO_READ_DATA_FLAG_DISCARD|, and |MOJO_READ_DATA_FLAG_ALL_OR_NONE|
//       is ignored if this flag is set.
//   |MOJO_READ_DATA_FLAG_PEEK| - Read elements without removing them. For use
//       with |MojoReadData()| only. Mutually exclusive with
//       |MOJO_READ_DATA_FLAG_DISCARD| and |MOJO_READ_DATA_FLAG_QUERY|.

typedef uint32_t MojoReadDataFlags;

#define MOJO_READ_DATA_FLAG_NONE ((MojoReadDataFlags)0)
#define MOJO_READ_DATA_FLAG_ALL_OR_NONE ((MojoReadDataFlags)1 << 0)
#define MOJO_READ_DATA_FLAG_DISCARD ((MojoReadDataFlags)1 << 1)
#define MOJO_READ_DATA_FLAG_QUERY ((MojoReadDataFlags)1 << 2)
#define MOJO_READ_DATA_FLAG_PEEK ((MojoReadDataFlags)1 << 3)

MOJO_BEGIN_EXTERN_C

// |MojoCreateDataPipe()|: Creates a data pipe, which is a unidirectional
// communication channel for unframed data, with the given options. Data is
// unframed, but must come as (multiples of) discrete elements, of the size
// given in |options|. See |MojoCreateDataPipeOptions| for a description of the
// different options available for data pipes.
//
// |options| may be set to null for a data pipe with the default options (which
// will have an element size of one byte and have some system-dependent
// capacity).
//
// On success, |*data_pipe_producer_handle| will be set to the handle for the
// producer and |*data_pipe_consumer_handle| will be set to the handle for the
// consumer. (On failure, they are not modified.) The producer handle will have
// (at least) the following rights: |MOJO_HANDLE_RIGHT_TRANSFER|,
// |MOJO_HANDLE_RIGHT_WRITE|, |MOJO_HANDLE_RIGHT_GET_OPTIONS|, and
// |MOJO_HANDLE_RIGHT_SET_OPTIONS|. The consumer handle will have (at least) the
// following rights: |MOJO_HANDLE_RIGHT_TRANSFER|, |MOJO_HANDLE_RIGHT_READ|,
// |MOJO_HANDLE_RIGHT_GET_OPTIONS|, and |MOJO_HANDLE_RIGHT_SET_OPTIONS|
//
// Returns:
//   |MOJO_RESULT_OK| on success.
//   |MOJO_RESULT_INVALID_ARGUMENT| if some argument was invalid (e.g.,
//       |*options| is invalid).
//   |MOJO_RESULT_RESOURCE_EXHAUSTED| if a process/system/quota/etc. limit has
//       been reached (e.g., if the requested capacity was too large, or if the
//       maximum number of handles was exceeded).
//   |MOJO_RESULT_UNIMPLEMENTED| if an unsupported flag was set in |*options|.
MojoResult MojoCreateDataPipe(
    const struct MojoCreateDataPipeOptions* MOJO_RESTRICT
        options,                                           // Optional in.
    MojoHandle* MOJO_RESTRICT data_pipe_producer_handle,   // Out.
    MojoHandle* MOJO_RESTRICT data_pipe_consumer_handle);  // Out.

// TODO(vtl): Probably should have a way of getting the
// |MojoCreateDataPipeOptions| (maybe just rename it to |MojoDataPipeOptions|?)
// from either handle as well.

// |MojoSetDataPipeProducerOptions()|: Sets options for the data pipe producer
// handle |data_pipe_producer_handle| (which must have the
// |MOJO_HANDLE_RIGHT_SET_OPTIONS| right).
//
// |options| may be set to null to reset back to the default options.
//
// Note that changing the write threshold may also result in the state (both
// satisfied or satisfiable) of the |MOJO_HANDLE_SIGNAL_WRITE_THRESHOLD| handle
// signal being changed.
//
// Returns:
//   |MOJO_RESULT_OK| on success.
//   |MOJO_RESULT_INVALID_ARGUMENT| if some argument was invalid (e.g.,
//       |data_pipe_producer_handle| is not a valid data pipe producer handle or
//       |*options| is invalid).
//   |MOJO_RESULT_PERMISSION_DENIED| if |data_pipe_producer_handle| does not
//       have the |MOJO_HANDLE_RIGHT_SET_OPTIONS| right.
//   |MOJO_RESULT_BUSY| if |data_pipe_producer_handle| is currently in use in
//       some transaction (that, e.g., may result in it being invalidated, such
//       as being sent in a message).
MojoResult MojoSetDataPipeProducerOptions(
    MojoHandle data_pipe_producer_handle,                // In.
    const struct MojoDataPipeProducerOptions* options);  // Optional in.

// |MojoGetDataPipeProducerOptions()|: Gets options for the data pipe producer
// handle |data_pipe_producer_handle| (which must have the
// |MOJO_HANDLE_RIGHT_GET_OPTIONS| right). |options| should be non-null and
// point to a buffer of size |options_num_bytes|; |options_num_bytes| should be
// at least 8 (the size of the first, and currently only, version of
// |MojoDataPipeProducerOptions|).
//
// On success, |*options| will be filled with information about the given
// buffer. Note that if additional (larger) versions of
// |MojoDataPipeProducerOptions| are defined, the largest version permitted by
// |options_num_bytes| that is supported by the implementation will be filled.
// Callers expecting more than the first 16-byte version must check the
// resulting |options->struct_size|.
//
// Returns:
//   |MOJO_RESULT_OK| on success.
//   |MOJO_RESULT_INVALID_ARGUMENT| if some argument was invalid (e.g.,
//       |data_pipe_producer_handle| is not a valid data pipe producer handle,
//       |*options| is null, or |options_num_bytes| is too small).
//   |MOJO_RESULT_PERMISSION_DENIED| if |data_pipe_producer_handle| does not
//       have the |MOJO_HANDLE_RIGHT_GET_OPTIONS| right.
//   |MOJO_RESULT_BUSY| if |data_pipe_producer_handle| is currently in use in
//       some transaction (that, e.g., may result in it being invalidated, such
//       as being sent in a message).
MojoResult MojoGetDataPipeProducerOptions(
    MojoHandle data_pipe_producer_handle,         // In.
    struct MojoDataPipeProducerOptions* options,  // Out.
    uint32_t options_num_bytes);                  // In.

// |MojoWriteData()|: Writes the given data to the data pipe producer given by
// |data_pipe_producer_handle| (which must have the |MOJO_HANDLE_RIGHT_WRITE|
// right). |elements| points to data of size |*num_bytes|; |*num_bytes| should
// be a multiple of the data pipe's element size. If
// |MOJO_WRITE_DATA_FLAG_ALL_OR_NONE| is set in |flags|, either all the data
// will be written or none is.
//
// On success, |*num_bytes| is set to the amount of data that was actually
// written.
//
// Returns:
//   |MOJO_RESULT_OK| on success.
//   |MOJO_RESULT_INVALID_ARGUMENT| if some argument was invalid (e.g.,
//       |data_pipe_producer_handle| is not a handle to a data pipe producer or
//       |*num_bytes| is not a multiple of the data pipe's element size).
//   |MOJO_RESULT_PERMISSION_DENIED| if |data_pipe_producer_handle| does not
//       have the |MOJO_HANDLE_RIGHT_WRITE| right.
//   |MOJO_RESULT_FAILED_PRECONDITION| if the data pipe consumer handle has been
//       closed.
//   |MOJO_RESULT_OUT_OF_RANGE| if |flags| has
//       |MOJO_WRITE_DATA_FLAG_ALL_OR_NONE| set and the required amount of data
//       (specified by |*num_bytes|) could not be written.
//   |MOJO_RESULT_BUSY| if |data_pipe_producer_handle| is currently in use in
//       some transaction (that, e.g., may result in it being invalidated, such
//       as being sent in a message), or if there is a two-phase write ongoing
//       with |data_pipe_producer_handle| (i.e., |MojoBeginWriteData()| has been
//       called, but not yet the matching |MojoEndWriteData()|).
//   |MOJO_RESULT_SHOULD_WAIT| if no data can currently be written (and the
//       consumer is still open) and |flags| does *not* have
//       |MOJO_WRITE_DATA_FLAG_ALL_OR_NONE| set.
//
// TODO(vtl): Should there be a way of querying how much data can be written?
MojoResult MojoWriteData(MojoHandle data_pipe_producer_handle,  // In.
                         const void* MOJO_RESTRICT elements,    // In.
                         uint32_t* MOJO_RESTRICT num_bytes,     // In/out.
                         MojoWriteDataFlags flags);             // In.

// |MojoBeginWriteData()|: Begins a two-phase write to the data pipe producer
// given by |data_pipe_producer_handle| (which must have the
// |MOJO_HANDLE_RIGHT_WRITE| right). On success, |*buffer| will be a pointer to
// which the caller can write |*buffer_num_bytes| bytes of data. There are
// currently no flags allowed, so |flags| should be |MOJO_WRITE_DATA_FLAG_NONE|.
//
// During a two-phase write, |data_pipe_producer_handle| is *not* writable.
// E.g., if another thread tries to write to it, it will get |MOJO_RESULT_BUSY|;
// that thread can then wait for |data_pipe_producer_handle| to become writable
// again.
//
// When |MojoBeginWriteData()| returns |MOJO_RESULT_OK|, and the caller has
// finished writing data to |*buffer|, it should call |MojoEndWriteData()| to
// specify the amount written and to complete the two-phase write.
// |MojoEndWriteData()| need not be called for other return values.
//
// Note: After a successful |MojoBeginWriteData()| on a given handle and before
// a corresponding |MojoEndWriteData()|, any operation that invalidates the
// handle (such as closing the handle, replacing the handle with one with
// reduced rights, or transferring the handle over a message pipe) will abort
// the two-phase write. That is, the behavior is equivalent to ending the
// two-phase write with no data written. That operation will also invalidate the
// buffer pointer: the behavior if data continues to be written to the buffer is
// undefined.
//
// Returns:
//   |MOJO_RESULT_OK| on success.
//   |MOJO_RESULT_INVALID_ARGUMENT| if some argument was invalid (e.g.,
//       |data_pipe_producer_handle| is not a handle to a data pipe producer or
//       flags has |MOJO_WRITE_DATA_FLAG_ALL_OR_NONE| set).
//   |MOJO_RESULT_PERMISSION_DENIED| if |data_pipe_producer_handle| does not
//       have the |MOJO_HANDLE_RIGHT_WRITE| right.
//   |MOJO_RESULT_FAILED_PRECONDITION| if the data pipe consumer handle has been
//       closed.
//   |MOJO_RESULT_BUSY| if |data_pipe_producer_handle| is currently in use in
//       some transaction (that, e.g., may result in it being invalidated, such
//       as being sent in a message), or if there is already a two-phase write
//       ongoing with |data_pipe_producer_handle| (i.e., |MojoBeginWriteData()|
//       has been called, but not yet the matching |MojoEndWriteData()|).
//   |MOJO_RESULT_SHOULD_WAIT| if no data can currently be written (and the
//       consumer is still open).
MojoResult MojoBeginWriteData(MojoHandle data_pipe_producer_handle,      // In.
                              void** MOJO_RESTRICT buffer,               // Out.
                              uint32_t* MOJO_RESTRICT buffer_num_bytes,  // Out.
                              MojoWriteDataFlags flags);                 // In.

// |MojoEndWriteData()|: Ends a two-phase write to the data pipe producer given
// by |data_pipe_producer_handle| (which must have the |MOJO_HANDLE_RIGHT_WRITE|
// right) that was begun by a call to |MojoBeginWriteData()| on the same handle.
// |num_bytes_written| should indicate the amount of data actually written; it
// must be less than or equal to the value of |*buffer_num_bytes| output by
// |MojoBeginWriteData()| and must be a multiple of the element size. The buffer
// given by |*buffer| from |MojoBeginWriteData()| must have been filled with
// exactly |num_bytes_written| bytes of data.
//
// On failure, the two-phase write (if any) is ended (so the handle may become
// writable again, if there's space available) but no data written to |*buffer|
// is "put into" the data pipe.
//
// Returns:
//   |MOJO_RESULT_OK| on success.
//   |MOJO_RESULT_INVALID_ARGUMENT| if some argument was invalid (e.g.,
//       |data_pipe_producer_handle| is not a handle to a data pipe producer or
//       |num_bytes_written| is invalid (greater than the maximum value provided
//       by |MojoBeginWriteData()| or not a multiple of the element size).
//   |MOJO_RESULT_PERMISSION_DENIED| if |data_pipe_producer_handle| does not
//       have the |MOJO_HANDLE_RIGHT_WRITE| right.
//   |MOJO_RESULT_FAILED_PRECONDITION| if the data pipe producer is not in a
//       two-phase write (e.g., |MojoBeginWriteData()| was not called or
//       |MojoEndWriteData()| has already been called).
//   |MOJO_RESULT_BUSY| if |data_pipe_producer_handle| is currently in use in
//       some transaction (that, e.g., may result in it being invalidated, such
//       as being sent in a message).
MojoResult MojoEndWriteData(MojoHandle data_pipe_producer_handle,  // In.
                            uint32_t num_bytes_written);           // In.

// |MojoSetDataPipeConsumerOptions()|: Sets options for the data pipe consumer
// handle |data_pipe_consumer_handle| (which must have the
// |MOJO_HANDLE_RIGHT_SET_OPTIONS| right).
//
// |options| may be set to null to reset back to the default options.
//
// Note that changing the read threshold may also result in the state (both
// satisfied or satisfiable) of the |MOJO_HANDLE_SIGNAL_READ_THRESHOLD| handle
// signal being changed.
//
// Returns:
//   |MOJO_RESULT_OK| on success.
//   |MOJO_RESULT_INVALID_ARGUMENT| if some argument was invalid (e.g.,
//       |data_pipe_consumer_handle| is not a valid data pipe consumer handle or
//       |*options| is invalid).
//   |MOJO_RESULT_PERMISSION_DENIED| if |data_pipe_consumer_handle| does not
//       have the |MOJO_HANDLE_RIGHT_SET_OPTIONS| right.
//   |MOJO_RESULT_BUSY| if |data_pipe_consumer_handle| is currently in use in
//       some transaction (that, e.g., may result in it being invalidated, such
//       as being sent in a message).
MojoResult MojoSetDataPipeConsumerOptions(
    MojoHandle data_pipe_consumer_handle,                // In.
    const struct MojoDataPipeConsumerOptions* options);  // Optional in.

// |MojoGetDataPipeConsumerOptions()|: Gets options for the data pipe consumer
// handle |data_pipe_consumer_handle| (which must have the
// |MOJO_HANDLE_RIGHT_GET_OPTIONS| right). |options| should be non-null and
// point to a buffer of size |options_num_bytes|; |options_num_bytes| should be
// at least 8 (the size of the first, and currently only, version of
// |MojoDataPipeConsumerOptions|).
//
// On success, |*options| will be filled with information about the given
// buffer. Note that if additional (larger) versions of
// |MojoDataPipeConsumerOptions| are defined, the largest version permitted by
// |options_num_bytes| that is supported by the implementation will be filled.
// Callers expecting more than the first 16-byte version must check the
// resulting |options->struct_size|.
//
// Returns:
//   |MOJO_RESULT_OK| on success.
//   |MOJO_RESULT_INVALID_ARGUMENT| if some argument was invalid (e.g.,
//       |data_pipe_consumer_handle| is not a valid data pipe consumer handle,
//       |*options| is null, or |options_num_bytes| is too small).
//   |MOJO_RESULT_PERMISSION_DENIED| if |data_pipe_consumer_handle| does not
//       have the |MOJO_HANDLE_RIGHT_GET_OPTIONS| right.
//   |MOJO_RESULT_BUSY| if |data_pipe_consumer_handle| is currently in use in
//       some transaction (that, e.g., may result in it being invalidated, such
//       as being sent in a message).
MojoResult MojoGetDataPipeConsumerOptions(
    MojoHandle data_pipe_consumer_handle,         // In.
    struct MojoDataPipeConsumerOptions* options,  // Out.
    uint32_t options_num_bytes);                  // In.

// |MojoReadData()|: Reads data from the data pipe consumer given by
// |data_pipe_consumer_handle| (which must have the |MOJO_HANDLE_RIGHT_READ|
// right). May also be used to discard data or query the amount of data
// available.
//
// If |flags| has neither |MOJO_READ_DATA_FLAG_DISCARD| nor
// |MOJO_READ_DATA_FLAG_QUERY| set, this tries to read up to |*num_bytes| (which
// must be a multiple of the data pipe's element size) bytes of data to
// |elements| and set |*num_bytes| to the amount actually read. If flags has
// |MOJO_READ_DATA_FLAG_ALL_OR_NONE| set, it will either read exactly
// |*num_bytes| bytes of data or none. Additionally, if flags has
// |MOJO_READ_DATA_FLAG_PEEK| set, the data read will remain in the pipe and be
// available to future reads.
//
// If flags has |MOJO_READ_DATA_FLAG_DISCARD| set, it discards up to
// |*num_bytes| (which again must be a multiple of the element size) bytes of
// data, setting |*num_bytes| to the amount actually discarded. If flags has
// |MOJO_READ_DATA_FLAG_ALL_OR_NONE|, it will either discard exactly
// |*num_bytes| bytes of data or none. In this case, |MOJO_READ_DATA_FLAG_QUERY|
// must not be set, and |elements| is ignored (and should typically be set to
// null).
//
// If flags has |MOJO_READ_DATA_FLAG_QUERY| set, it queries the amount of data
// available, setting |*num_bytes| to the number of bytes available. In this
// case, |MOJO_READ_DATA_FLAG_DISCARD| must not be set, and
// |MOJO_READ_DATA_FLAG_ALL_OR_NONE| is ignored, as are |elements| and the input
// value of |*num_bytes|.
//
// Returns:
//   |MOJO_RESULT_OK| on success (see above for a description of the different
//       operations).
//   |MOJO_RESULT_INVALID_ARGUMENT| if some argument was invalid (e.g.,
//       |data_pipe_consumer_handle| is invalid, the combination of flags in
//       |flags| is invalid, etc.).
//   |MOJO_RESULT_PERMISSION_DENIED| if |data_pipe_consumer_handle| does not
//       have the |MOJO_HANDLE_RIGHT_READ| right.
//   |MOJO_RESULT_FAILED_PRECONDITION| if the data pipe producer handle has been
//       closed and data (or the required amount of data) was not available to
//       be read or discarded.
//   |MOJO_RESULT_OUT_OF_RANGE| if |flags| has |MOJO_READ_DATA_FLAG_ALL_OR_NONE|
//       set and the required amount of data is not available to be read or
//       discarded (and the producer is still open).
//   |MOJO_RESULT_BUSY| if |data_pipe_consumer_handle| is currently in use in
//       some transaction (that, e.g., may result in it being invalidated, such
//       as being sent in a message), or if there is a two-phase read ongoing
//       with |data_pipe_consumer_handle| (i.e., |MojoBeginReadData()| has been
//       called, but not yet the matching |MojoEndReadData()|).
//   |MOJO_RESULT_SHOULD_WAIT| if there is no data to be read or discarded (and
//       the producer is still open) and |flags| does *not* have
//       |MOJO_READ_DATA_FLAG_ALL_OR_NONE| set.
MojoResult MojoReadData(MojoHandle data_pipe_consumer_handle,  // In.
                        void* MOJO_RESTRICT elements,          // Out.
                        uint32_t* MOJO_RESTRICT num_bytes,     // In/out.
                        MojoReadDataFlags flags);              // In.

// |MojoBeginReadData()|: Begins a two-phase read from the data pipe consumer
// given by |data_pipe_consumer_handle| (which must have the
// |MOJO_HANDLE_RIGHT_READ| right). On success, |*buffer| will be a pointer from
// which the caller can read |*buffer_num_bytes| bytes of data. There are
// currently no valid flags, so |flags| must be |MOJO_READ_DATA_FLAG_NONE|.
//
// During a two-phase read, |data_pipe_consumer_handle| is *not* readable.
// E.g., if another thread tries to read from it, it will get
// |MOJO_RESULT_BUSY|; that thread can then wait for |data_pipe_consumer_handle|
// to become readable again.
//
// Once the caller has finished reading data from |*buffer|, it should call
// |MojoEndReadData()| to specify the amount read and to complete the two-phase
// read.
//
// Note: After a successful |MojoBeginReadData()| on a given handle and before a
// corresponding |MojoEndReadData()|, any operation that invalidates the handle
// (such as closing the handle, replacing the handle with one with reduced
// rights, or transferring the handle over a message pipe) will abort the
// two-phase read. That is, the behavior is equivalent to ending the two-phase
// read with no data consumed. That operation will also invalidate the buffer
// pointer: the behavior if data continues to be read from the buffer is
// undefined.
//
// Returns:
//   |MOJO_RESULT_OK| on success.
//   |MOJO_RESULT_INVALID_ARGUMENT| if some argument was invalid (e.g.,
//       |data_pipe_consumer_handle| is not a handle to a data pipe consumer,
//       or |flags| has invalid flags set).
//   |MOJO_RESULT_PERMISSION_DENIED| if |data_pipe_consumer_handle| does not
//       have the |MOJO_HANDLE_RIGHT_READ| right.
//   |MOJO_RESULT_FAILED_PRECONDITION| if the data pipe producer handle has been
//       closed.
//   |MOJO_RESULT_BUSY| if |data_pipe_consumer_handle| is currently in use in
//       some transaction (that, e.g., may result in it being invalidated, such
//       as being sent in a message), or if there is already a two-phase read
//       ongoing with |data_pipe_consumer_handle| (i.e., |MojoBeginReadData()|
//       has been called, but not yet the matching |MojoEndReadData()|).
//   |MOJO_RESULT_SHOULD_WAIT| if no data can currently be read (and the
//       producer is still open).
MojoResult MojoBeginReadData(MojoHandle data_pipe_consumer_handle,      // In.
                             const void** MOJO_RESTRICT buffer,         // Out.
                             uint32_t* MOJO_RESTRICT buffer_num_bytes,  // Out.
                             MojoReadDataFlags flags);                  // In.

// |MojoEndReadData()|: Ends a two-phase read from the data pipe consumer given
// by |data_pipe_consumer_handle| (which must have the |MOJO_HANDLE_RIGHT_READ|
// right) that was begun by a call to |MojoBeginReadData()| on the same handle.
// |num_bytes_read| should indicate the amount of data actually read; it must be
// less than or equal to the value of |*buffer_num_bytes| output by
// |MojoBeginReadData()| and must be a multiple of the element size.
//
// On failure, the two-phase read (if any) is ended (so the handle may become
// readable again) but no data is "removed" from the data pipe.
//
// Returns:
//   |MOJO_RESULT_OK| on success.
//   |MOJO_RESULT_INVALID_ARGUMENT| if some argument was invalid (e.g.,
//       |data_pipe_consumer_handle| is not a handle to a data pipe consumer or
//       |num_bytes_written| is greater than the maximum value provided by
//       |MojoBeginReadData()| or not a multiple of the element size).
//   |MOJO_RESULT_PERMISSION_DENIED| if |data_pipe_consumer_handle| does not
//       have the |MOJO_HANDLE_RIGHT_READ| right.
//   |MOJO_RESULT_FAILED_PRECONDITION| if the data pipe consumer is not in a
//       two-phase read (e.g., |MojoBeginReadData()| was not called or
//       |MojoEndReadData()| has already been called).
//   |MOJO_RESULT_BUSY| if |data_pipe_consumer_handle| is currently in use in
//       some transaction (that, e.g., may result in it being invalidated, such
//       as being sent in a message).
MojoResult MojoEndReadData(MojoHandle data_pipe_consumer_handle,  // In.
                           uint32_t num_bytes_read);              // In.

MOJO_END_EXTERN_C

#endif  // MOJO_PUBLIC_C_SYSTEM_DATA_PIPE_H_
