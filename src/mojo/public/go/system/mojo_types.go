// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package system

import "math"

// Go equivalent definitions of the various system types defined in Mojo.
//
type MojoTimeTicks int64
type MojoHandle uint32
type MojoResult uint32
type MojoDeadline uint64
type MojoHandleSignals uint32
type MojoWriteMessageFlags uint32
type MojoReadMessageFlags uint32
type MojoWriteDataFlags uint32
type MojoReadDataFlags uint32
type MojoCreateDataPipeOptionsFlags uint32
type MojoCreateMessagePipeOptionsFlags uint32
type MojoCreateSharedBufferOptionsFlags uint32
type MojoDuplicateBufferHandleOptionsFlags uint32
type MojoMapBufferFlags uint32
type MojoBufferInformationFlags uint32

const (
	MOJO_DEADLINE_INDEFINITE        MojoDeadline = math.MaxUint64
	MOJO_HANDLE_INVALID             MojoHandle   = 0
  // TODO(vtl): Find a way of supporting the new, more flexible/extensible
  // MojoResult (see mojo/public/c/syste/result.h).
	MOJO_RESULT_OK                  MojoResult   = 0x0
	MOJO_RESULT_CANCELLED           MojoResult   = 0x1
	MOJO_RESULT_UNKNOWN             MojoResult   = 0x2
	MOJO_RESULT_INVALID_ARGUMENT    MojoResult   = 0x3
	MOJO_RESULT_DEADLINE_EXCEEDED   MojoResult   = 0x4
	MOJO_RESULT_NOT_FOUND           MojoResult   = 0x5
	MOJO_RESULT_ALREADY_EXISTS      MojoResult   = 0x6
	MOJO_RESULT_PERMISSION_DENIED   MojoResult   = 0x7
	MOJO_RESULT_RESOURCE_EXHAUSTED  MojoResult   = 0x8
	MOJO_RESULT_FAILED_PRECONDITION MojoResult   = 0x9
	MOJO_RESULT_ABORTED             MojoResult   = 0xa
	MOJO_RESULT_OUT_OF_RANGE        MojoResult   = 0xb
	MOJO_RESULT_UNIMPLEMENTED       MojoResult   = 0xc
	MOJO_RESULT_INTERNAL            MojoResult   = 0xd
	MOJO_RESULT_UNAVAILABLE         MojoResult   = 0xe
	MOJO_RESULT_DATA_LOSS           MojoResult   = 0xf
  // MOJO_RESULT_FAILED_PRECONDITION, subcode 0x001:
	MOJO_RESULT_BUSY                MojoResult   = 0x0019
  // MOJO_RESULT_UNAVAILABLE, subcode 0x001:
	MOJO_RESULT_SHOULD_WAIT         MojoResult   = 0x001e

	MOJO_HANDLE_SIGNAL_NONE        MojoHandleSignals = 0
	MOJO_HANDLE_SIGNAL_READABLE    MojoHandleSignals = 1 << 0
	MOJO_HANDLE_SIGNAL_WRITABLE    MojoHandleSignals = 1 << 1
	MOJO_HANDLE_SIGNAL_PEER_CLOSED MojoHandleSignals = 1 << 2

	MOJO_WRITE_MESSAGE_FLAG_NONE       MojoWriteMessageFlags = 0
	MOJO_READ_MESSAGE_FLAG_NONE        MojoReadMessageFlags  = 0
	MOJO_READ_MESSAGE_FLAG_MAY_DISCARD MojoReadMessageFlags  = 1 << 0

	MOJO_READ_DATA_FLAG_NONE         MojoReadDataFlags  = 0
	MOJO_READ_DATA_FLAG_ALL_OR_NONE  MojoReadDataFlags  = 1 << 0
	MOJO_READ_DATA_FLAG_DISCARD      MojoReadDataFlags  = 1 << 1
	MOJO_READ_DATA_FLAG_QUERY        MojoReadDataFlags  = 1 << 2
	MOJO_READ_DATA_FLAG_PEEK         MojoReadDataFlags  = 1 << 3
	MOJO_WRITE_DATA_FLAG_NONE        MojoWriteDataFlags = 0
	MOJO_WRITE_DATA_FLAG_ALL_OR_NONE MojoWriteDataFlags = 1 << 0

	MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE    MojoCreateDataPipeOptionsFlags    = 0
	MOJO_CREATE_MESSAGE_PIPE_OPTIONS_FLAG_NONE MojoCreateMessagePipeOptionsFlags = 0

	MOJO_CREATE_SHARED_BUFFER_OPTIONS_FLAG_NONE    MojoCreateSharedBufferOptionsFlags    = 0
	MOJO_DUPLICATE_BUFFER_HANDLE_OPTIONS_FLAG_NONE MojoDuplicateBufferHandleOptionsFlags = 0
	MOJO_MAP_BUFFER_FLAG_NONE                      MojoMapBufferFlags                    = 0
	MOJO_BUFFER_INFORMATION_FLAG_NONE              MojoBufferInformationFlags            = 0
)

// IsReadable returns true iff the |MOJO_HANDLE_SIGNAL_READABLE| bit is set.
func (m MojoHandleSignals) IsReadable() bool {
	return (m & MOJO_HANDLE_SIGNAL_READABLE) != 0
}

// IsWritable returns true iff the |MOJO_HANDLE_SIGNAL_WRITABLE| bit is set.
func (m MojoHandleSignals) IsWritable() bool {
	return (m & MOJO_HANDLE_SIGNAL_WRITABLE) != 0
}

// IsClosed returns true iff the |MOJO_HANDLE_SIGNAL_PEER_CLOSED| bit is set.
func (m MojoHandleSignals) IsClosed() bool {
	return (m & MOJO_HANDLE_SIGNAL_PEER_CLOSED) != 0
}

// MojoHandleSignalsState is a struct returned by wait functions to indicate
// the signaling state of handles.
type MojoHandleSignalsState struct {
	// Signals that were satisfied at some time before the call returned.
	SatisfiedSignals MojoHandleSignals
	// Signals that are possible to satisfy. For example, if the return value
	// was |MOJO_RESULT_FAILED_PRECONDITION|, you can use this field to
	// determine which, if any, of the signals can still be satisfied.
	SatisfiableSignals MojoHandleSignals
}

// DataPipeOptions is used to specify creation parameters for a data pipe.
type DataPipeOptions struct {
	Flags MojoCreateDataPipeOptionsFlags
	// The size of an element in bytes. All transactions and buffers will
	// be an integral number of elements.
	ElemSize uint32
	// The capacity of the data pipe in bytes. Must be a multiple of elemSize.
	Capacity uint32
}

// MessagePipeOptions is used to specify creation parameters for a message pipe.
type MessagePipeOptions struct {
	Flags MojoCreateMessagePipeOptionsFlags
}

// SharedBufferOptions is used to specify creation parameters for a
// shared buffer.
type SharedBufferOptions struct {
	Flags MojoCreateSharedBufferOptionsFlags
}

// DuplicateBufferHandleOptions is used to specify parameters in
// duplicating access to a shared buffer.
type DuplicateBufferHandleOptions struct {
	Flags MojoDuplicateBufferHandleOptionsFlags
}

// MojoBufferInformation is returned by the GetBufferInformation system
// call.
type MojoBufferInformation struct {
	// Possible values:
	//   MOJO_BUFFER_INFORMATION_FLAG_NONE
	Flags MojoBufferInformationFlags

	// The size of this shared buffer, in bytes.
	NumBytes uint64
}
