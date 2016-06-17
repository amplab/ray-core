// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Define a set of C++ specific macros.
// Mojo C++ API users can assume that mojo/public/cpp/system/macros.h
// includes mojo/public/c/system/macros.h.

#ifndef MOJO_PUBLIC_CPP_SYSTEM_MACROS_H_
#define MOJO_PUBLIC_CPP_SYSTEM_MACROS_H_

#include <utility>

#include "mojo/public/c/system/macros.h"  // Symbols exposed.

// A macro to disallow the copy constructor and operator= functions. This is
// typically used like:
//
//   class MyUncopyableClass {
//    public:
//     ...
//    private:
//     ...
//     MOJO_DISALLOW_COPY_AND_ASSIGN(MyUncopyableClass);
//   };
#define MOJO_DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&) = delete;           \
  void operator=(const TypeName&) = delete

// Used to calculate the number of elements in an array.
// (See |arraysize()| in Chromium's base/macros.h for more details.)
namespace mojo {
namespace internal {
template <typename T, size_t N>
char(&ArraySizeHelper(T(&array)[N]))[N];
#if !defined(_MSC_VER)
template <typename T, size_t N>
char(&ArraySizeHelper(const T(&array)[N]))[N];
#endif
}  // namespace internal
}  // namespace mojo
#define MOJO_ARRAYSIZE(array) (sizeof(::mojo::internal::ArraySizeHelper(array)))

// Used to make a type move-only. (The MoveOnlyTypeForCPP03 typedef is for
// Chromium's base/callback.h to tell that this type is move-only.) This is
// typically used like:
//
//   class MyMoveOnlyClass {
//    public:
//     ...
//    private:
//     ...
//     MOJO_MOVE_ONLY_TYPE(MyMoveOnlyClass);
//   };
//
// (Note: Class members following the use of this macro will have private access
// by default.)
#define MOJO_MOVE_ONLY_TYPE(type)                                    \
 public:                                                             \
  type&& Pass() MOJO_WARN_UNUSED_RESULT { return std::move(*this); } \
  typedef void MoveOnlyTypeForCPP03;                                 \
                                                                     \
 private:                                                            \
  type(const type&) = delete;                                        \
  void operator=(const type&) = delete

namespace mojo {

// Used to explicitly mark the return value of a function as unused. (Use this
// if you are really sure you don't want to do anything with the return value of
// a function marked with |MOJO_WARN_UNUSED_RESULT|.
template <typename T>
inline void ignore_result(const T&) {
}

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_SYSTEM_MACROS_H_
