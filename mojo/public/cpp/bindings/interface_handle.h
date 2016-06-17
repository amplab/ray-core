// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_INTERFACE_PTR_INFO_H_
#define MOJO_PUBLIC_CPP_BINDINGS_INTERFACE_PTR_INFO_H_

#include <cstddef>

#include "mojo/public/cpp/system/macros.h"
#include "mojo/public/cpp/system/message_pipe.h"

namespace mojo {

template <typename Interface>
class InterfacePtr;

template <typename Interface>
class SynchronousInterfacePtr;

// InterfaceHandle stores necessary information to communicate with a remote
// interface implementation, which could be used to construct an InterfacePtr.
template <typename Interface>
class InterfaceHandle {
 public:
  InterfaceHandle() : version_(0u) {}
  InterfaceHandle(std::nullptr_t) : version_(0u) {}

  InterfaceHandle(ScopedMessagePipeHandle handle, uint32_t version)
      : handle_(handle.Pass()), version_(version) {}

  InterfaceHandle(InterfaceHandle&& other)
      : handle_(other.handle_.Pass()), version_(other.version_) {
    other.version_ = 0u;
  }

  // Making this constructor templated ensures that it is not type-instantiated
  // unless it is used, making the InterfacePtr<->InterfaceHandle codependency
  // less fragile.
  template <typename SameInterfaceAsAbove = Interface>
  InterfaceHandle(InterfacePtr<SameInterfaceAsAbove>&& ptr) {
    *this = ptr.PassInterfaceHandle();
  }

  // Making this constructor templated ensures that it is not type-instantiated
  // unless it is used, making the SynchronousInterfacePtr<->InterfaceHandle
  // codependency less fragile.
  template <typename SameInterfaceAsAbove = Interface>
  InterfaceHandle(SynchronousInterfacePtr<SameInterfaceAsAbove>&& ptr) {
    *this = ptr.PassInterfaceHandle();
  }

  ~InterfaceHandle() {}

  InterfaceHandle& operator=(InterfaceHandle&& other) {
    if (this != &other) {
      handle_ = other.handle_.Pass();
      version_ = other.version_;
      other.version_ = 0u;
    }

    return *this;
  }

  // Tests as true if we have a valid handle.
  explicit operator bool() const { return is_valid(); }
  bool is_valid() const { return handle_.is_valid(); }

  ScopedMessagePipeHandle PassHandle() { return handle_.Pass(); }
  const ScopedMessagePipeHandle& handle() const { return handle_; }
  void set_handle(ScopedMessagePipeHandle handle) { handle_ = handle.Pass(); }

  uint32_t version() const { return version_; }
  void set_version(uint32_t version) { version_ = version; }

 private:
  ScopedMessagePipeHandle handle_;
  uint32_t version_;

  MOJO_MOVE_ONLY_TYPE(InterfaceHandle);
};

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_INTERFACE_PTR_INFO_H_
