// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Headers such as array_serialization.h can include this file to avoid circular
// dependencies on map_serialization.h.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_LIB_MAP_SERIALIZATION_FORWARD_H_
#define MOJO_PUBLIC_CPP_BINDINGS_LIB_MAP_SERIALIZATION_FORWARD_H_

namespace mojo {
namespace internal {

class ArrayValidateParams;
class Buffer;

template <typename Key, typename Value>
class Map_Data;

}  // namespace internal

template <typename Key, typename Value>
class Map;

template <typename MapKey,
          typename MapValue,
          typename DataKey,
          typename DataValue>
internal::ValidationError SerializeMap_(
    Map<MapKey, MapValue>* input,
    internal::Buffer* buf,
    internal::Map_Data<DataKey, DataValue>** output,
    const internal::ArrayValidateParams* value_validate_params);
template <typename MapKey, typename MapValue>
size_t GetSerializedSize_(const Map<MapKey, MapValue>& input);

template <typename MapKey,
          typename MapValue,
          typename DataKey,
          typename DataValue>
void Deserialize_(internal::Map_Data<DataKey, DataValue>* input,
                  Map<MapKey, MapValue>* output);

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_LIB_MAP_SERIALIZATION_FORWARD_H_
