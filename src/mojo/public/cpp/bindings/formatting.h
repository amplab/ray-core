// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_FORMATTING_H_
#define MOJO_PUBLIC_CPP_BINDINGS_FORMATTING_H_

#include <iosfwd>

#include "mojo/public/cpp/bindings/array.h"
#include "mojo/public/cpp/bindings/map.h"
#include "mojo/public/cpp/bindings/struct_ptr.h"

namespace mojo {

// Prints the contents of an array to an output stream in a human-readable
// format.
template <typename T>
std::ostream& operator<<(std::ostream& os, const mojo::Array<T>& array) {
  if (array) {
    os << "[";
    bool first = true;
    for (auto it = array.storage().cbegin(); it != array.storage().cend();
         ++it) {
      if (first)
        first = false;
      else
        os << ", ";
      os << *it;
    }
    os << "]";
  } else {
    os << "null";
  }
  return os;
}

// Prints the contents of a map to an output stream in a human-readable
// format.
template <typename Key, typename Value>
std::ostream& operator<<(std::ostream& os, const mojo::Map<Key, Value>& map) {
  if (map) {
    os << "{";
    bool first = true;
    for (auto it = map.cbegin(); it != map.cend(); ++it) {
      if (first)
        first = false;
      else
        os << ", ";
      os << it.GetKey() << ": " << it.GetValue();
    }
    os << "}";
  } else {
    os << "null";
  }
  return os;
}

// Prints the pointee of a Mojo structure pointer to an output stream
// assuming there exists an operator<< overload that accepts a const
// reference to the object.
template <typename T, typename = typename T::Data_>
auto operator<<(std::ostream& os, const T* value) -> decltype(os << *value) {
  return value ? os << *value : os << "null";
}
template <typename T>
std::ostream& operator<<(std::ostream& os, const StructPtr<T>& value) {
  return os << value.get();
}
template <typename T>
std::ostream& operator<<(std::ostream& os, const InlinedStructPtr<T>& value) {
  return os << value.get();
}

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_FORMATTING_H_
