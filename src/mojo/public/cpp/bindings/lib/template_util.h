// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_LIB_TEMPLATE_UTIL_H_
#define MOJO_PUBLIC_CPP_BINDINGS_LIB_TEMPLATE_UTIL_H_

#include <type_traits>

namespace mojo {
namespace internal {

// Types YesType and NoType are guaranteed such that sizeof(YesType) <
// sizeof(NoType).
typedef char YesType;

struct NoType {
  YesType dummy[2];
};

// A helper template to determine if given type is non-const move-only-type,
// i.e. if a value of the given type should be passed via .Pass() in a
// destructive way.
template <typename T>
struct IsMoveOnlyType {
  template <typename U>
  static YesType Test(const typename U::MoveOnlyTypeForCPP03*);

  template <typename U>
  static NoType Test(...);

  static const bool value =
      sizeof(Test<T>(0)) == sizeof(YesType) && !std::is_const<T>::value;
};

// Returns a reference to |t| when T is not a move-only type.
template <typename T>
typename std::enable_if<!IsMoveOnlyType<T>::value, T>::type& Forward(T& t) {
  return t;
}

// Returns the result of t.Pass() when T is a move-only type.
template <typename T>
typename std::enable_if<IsMoveOnlyType<T>::value, T>::type Forward(T& t) {
  return t.Pass();
}

template <template <typename...> class Template, typename T>
struct IsSpecializationOf : std::false_type {};

template <template <typename...> class Template, typename... Args>
struct IsSpecializationOf<Template, Template<Args...>> : std::true_type {};

}  // namespace internal
}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_LIB_TEMPLATE_UTIL_H_
