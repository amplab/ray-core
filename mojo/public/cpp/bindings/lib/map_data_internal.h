// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_LIB_MAP_DATA_INTERNAL_H_
#define MOJO_PUBLIC_CPP_BINDINGS_LIB_MAP_DATA_INTERNAL_H_

#include <string>
#include <vector>

#include "mojo/public/cpp/bindings/lib/array_internal.h"
#include "mojo/public/cpp/bindings/lib/validate_params.h"
#include "mojo/public/cpp/bindings/lib/validation_errors.h"
#include "mojo/public/cpp/bindings/lib/validation_util.h"

namespace mojo {
namespace internal {

inline const ArrayValidateParams* GetMapKeyValidateParamsDefault() {
  // The memory allocated here never gets released to not cause an exit time
  // destructor.
  static const ArrayValidateParams* validate_params =
      new ArrayValidateParams(0, false, nullptr);
  return validate_params;
}

inline const ArrayValidateParams* GetMapKeyValidateParamsForStrings() {
  // The memory allocated here never gets released to not cause an exit time
  // destructor.
  static const ArrayValidateParams* validate_params = new ArrayValidateParams(
      0, false, new ArrayValidateParams(0, false, nullptr));
  return validate_params;
}

template <typename MapKey>
struct MapKeyValidateParamsFactory {
  static const ArrayValidateParams* Get() {
    return GetMapKeyValidateParamsDefault();
  }
};

// For non-nullable strings only. (Which is OK; map keys can't be null.)
template <>
struct MapKeyValidateParamsFactory<mojo::internal::Array_Data<char>*> {
  static const ArrayValidateParams* Get() {
    return GetMapKeyValidateParamsForStrings();
  }
};

// Map serializes into a struct which has two arrays as struct fields, the keys
// and the values.
// TODO(vardhan): Fill out the missing validation error messages.
template <typename Key, typename Value>
class Map_Data {
 public:
  static Map_Data* New(Buffer* buf) {
    return new (buf->Allocate(sizeof(Map_Data))) Map_Data();
  }

  static ValidationError Validate(
      const void* data,
      BoundsChecker* bounds_checker,
      const ArrayValidateParams* value_validate_params,
      std::string* err) {
    if (!data)
      return ValidationError::NONE;

    ValidationError retval =
        ValidateStructHeaderAndClaimMemory(data, bounds_checker, err);
    if (retval != ValidationError::NONE)
      return retval;

    const Map_Data* object = static_cast<const Map_Data*>(data);
    if (object->header_.num_bytes != sizeof(Map_Data) ||
        object->header_.version != 0) {
      MOJO_INTERNAL_DEBUG_SET_ERROR_MSG(err) << "";
      return ValidationError::UNEXPECTED_STRUCT_HEADER;
    }

    if (!ValidateEncodedPointer(&object->keys.offset)) {
      MOJO_INTERNAL_DEBUG_SET_ERROR_MSG(err) << "";
      return ValidationError::ILLEGAL_POINTER;
    }

    if (!object->keys.offset) {
      MOJO_INTERNAL_DEBUG_SET_ERROR_MSG(err) << "null key array in map struct";
      return ValidationError::UNEXPECTED_NULL_POINTER;
    }

    const ArrayValidateParams* key_validate_params =
        MapKeyValidateParamsFactory<Key>::Get();
    retval =
        Array_Data<Key>::Validate(DecodePointerRaw(&object->keys.offset),
                                  bounds_checker, key_validate_params, err);
    if (retval != ValidationError::NONE)
      return retval;

    if (!ValidateEncodedPointer(&object->values.offset)) {
      MOJO_INTERNAL_DEBUG_SET_ERROR_MSG(err) << "";
      return ValidationError::ILLEGAL_POINTER;
    }

    if (!object->values.offset) {
      MOJO_INTERNAL_DEBUG_SET_ERROR_MSG(err)
          << "null value array in map struct";
      return ValidationError::UNEXPECTED_NULL_POINTER;
    }

    retval =
        Array_Data<Value>::Validate(DecodePointerRaw(&object->values.offset),
                                    bounds_checker, value_validate_params, err);
    if (retval != ValidationError::NONE)
      return retval;

    const ArrayHeader* key_header =
        static_cast<const ArrayHeader*>(DecodePointerRaw(&object->keys.offset));
    const ArrayHeader* value_header = static_cast<const ArrayHeader*>(
        DecodePointerRaw(&object->values.offset));
    if (key_header->num_elements != value_header->num_elements) {
      MOJO_INTERNAL_DEBUG_SET_ERROR_MSG(err) << "";
      return ValidationError::DIFFERENT_SIZED_ARRAYS_IN_MAP;
    }

    return ValidationError::NONE;
  }

  StructHeader header_;

  ArrayPointer<Key> keys;
  ArrayPointer<Value> values;

  void EncodePointersAndHandles(std::vector<mojo::Handle>* handles) {
    Encode(&keys, handles);
    Encode(&values, handles);
  }

  void DecodePointersAndHandles(std::vector<mojo::Handle>* handles) {
    Decode(&keys, handles);
    Decode(&values, handles);
  }

 private:
  Map_Data() {
    header_.num_bytes = sizeof(*this);
    header_.version = 0;
  }
  ~Map_Data() = delete;
};
static_assert(sizeof(Map_Data<char, char>) == 24, "Bad sizeof(Map_Data)");

}  // namespace internal
}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_LIB_MAP_DATA_INTERNAL_H_
