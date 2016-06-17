// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>
#include <string.h>

#include "base/logging.h"
#include "mojo/dart/embedder/common.h"
#include "mojo/public/cpp/environment/logging.h"

namespace mojo {
namespace dart {

Dart_Handle DartEmbedder::GetDartType(const char* library_url,
                                      const char* class_name) {
  return Dart_GetType(Dart_LookupLibrary(NewCString(library_url)),
                      NewCString(class_name), 0, NULL);
}

Dart_Handle DartEmbedder::NewDartExceptionWithMessage(
    const char* library_url,
    const char* error_type,
    const char* message) {
  // Create a Dart Exception object with a message.
  Dart_Handle type = GetDartType(library_url, error_type);
  DCHECK(!Dart_IsError(type));
  if (message != NULL) {
    Dart_Handle args[1];
    args[0] = NewCString(message);
    return Dart_New(type, Dart_Null(), 1, args);
  } else {
    return Dart_New(type, Dart_Null(), 0, NULL);
  }
}

Dart_Handle DartEmbedder::NewInternalError(const char* message) {
  return NewDartExceptionWithMessage("dart:core", "_InternalError", message);
}

int64_t DartEmbedder::GetIntegerValue(Dart_Handle value_obj) {
  int64_t value = 0;
  Dart_Handle result = Dart_IntegerToInt64(value_obj, &value);
  if (Dart_IsError(result)) {
    Dart_PropagateError(result);
  }
  return value;
}

int64_t DartEmbedder::GetInt64ValueCheckRange(
    Dart_Handle value_obj, int64_t lower, int64_t upper) {
  int64_t value = DartEmbedder::GetIntegerValue(value_obj);
  if (value < lower || upper < value) {
    Dart_PropagateError(Dart_NewApiError("Value outside expected range"));
  }
  return value;
}

intptr_t DartEmbedder::GetIntptrValue(Dart_Handle value_obj) {
  int64_t value = 0;
  Dart_Handle result = Dart_IntegerToInt64(value_obj, &value);
  if (Dart_IsError(result)) {
    Dart_PropagateError(result);
  }
  if (value < INTPTR_MIN || INTPTR_MAX < value) {
    Dart_PropagateError(Dart_NewApiError("Value outside intptr_t range"));
  }
  return static_cast<intptr_t>(value);
}

const char* DartEmbedder::GetStringValue(Dart_Handle str_obj) {
  const char* cstring = nullptr;
  Dart_Handle result = Dart_StringToCString(str_obj, &cstring);
  if (Dart_IsError(result)) {
    Dart_PropagateError(result);
  }
  return cstring;
}

bool DartEmbedder::GetBooleanValue(Dart_Handle bool_obj) {
  bool value = false;
  Dart_Handle result = Dart_BooleanValue(bool_obj, &value);
  if (Dart_IsError(result)) {
    Dart_PropagateError(result);
  }
  return value;
}

void DartEmbedder::SetIntegerField(Dart_Handle handle,
                                   const char* name,
                                   int64_t val) {
  Dart_Handle result = Dart_SetField(handle,
                                     NewCString(name),
                                     Dart_NewInteger(val));
  if (Dart_IsError(result)) {
    Dart_PropagateError(result);
  }
}

void DartEmbedder::SetStringField(Dart_Handle handle,
                                  const char* name,
                                  const char* val) {
  Dart_Handle result = Dart_SetField(handle, NewCString(name), NewCString(val));
  if (Dart_IsError(result)) {
    Dart_PropagateError(result);
  }
}

Dart_Handle DartEmbedder::NewCString(const char* str) {
  MOJO_CHECK(str != nullptr);
  Dart_Handle result = Dart_NewStringFromCString(str);
  if (Dart_IsError(result)) {
    Dart_PropagateError(result);
  }
  return result;
}

void DartEmbedder::SetNullReturn(Dart_NativeArguments arguments) {
  Dart_SetReturnValue(arguments, Dart_Null());
}

void DartEmbedder::SetCStringReturn(Dart_NativeArguments arguments,
                                    const char* str) {
  Dart_SetReturnValue(arguments, NewCString(str));
}

bool DartEmbedder::GetBooleanArgument(Dart_NativeArguments args,
                                      intptr_t index) {
  bool result;
  Dart_Handle err = Dart_GetNativeBooleanArgument(args, index, &result);
  if (Dart_IsError(err)) {
    Dart_PropagateError(err);
  }
  return result;
}

int64_t DartEmbedder::GetIntegerArgument(Dart_NativeArguments args,
                                         intptr_t index) {
  int64_t result;
  Dart_Handle err = Dart_GetNativeIntegerArgument(args, index, &result);
  if (Dart_IsError(err)) {
    Dart_PropagateError(err);
  }
  return result;
}

intptr_t DartEmbedder::GetIntptrArgument(Dart_NativeArguments args,
                                         intptr_t index) {
  int64_t value = 0;
  Dart_Handle result = Dart_GetNativeIntegerArgument(args, index, &value);
  if (Dart_IsError(result)) {
    Dart_PropagateError(result);
  }
  if (value < INTPTR_MIN || INTPTR_MAX < value) {
    Dart_PropagateError(Dart_NewApiError("Value outside intptr_t range"));
  }
  return static_cast<intptr_t>(value);
}

const char* DartEmbedder::GetStringArgument(Dart_NativeArguments arguments,
                                            intptr_t index) {
  Dart_Handle str_arg = Dart_GetNativeArgument(arguments, index);
  const char* cstring = nullptr;
  Dart_Handle result = Dart_StringToCString(str_arg, &cstring);
  if (Dart_IsError(result)) {
    Dart_PropagateError(result);
  }
  return cstring;
}

void DartEmbedder::GetTypedDataListArgument(Dart_NativeArguments arguments,
                                            intptr_t index,
                                            uint8_t** out,
                                            intptr_t* out_len) {
  // Initialize outputs.
  *out = nullptr;
  *out_len = 0;
  Dart_Handle list_arg = Dart_GetNativeArgument(arguments, index);
  if (Dart_IsError(list_arg)) {
    Dart_PropagateError(list_arg);
    return;
  }
  // Acquire data.
  Dart_TypedData_Type type = Dart_TypedData_kInvalid;
  void* data = nullptr;
  intptr_t data_len = 0;
  Dart_Handle result =
      Dart_TypedDataAcquireData(list_arg, &type, &data, &data_len);
  if (Dart_IsError(result)) {
    Dart_PropagateError(result);
    return;
  }
  // Only support Uint8List for now.
  if (type != Dart_TypedData_kUint8) {
    // Release data.
    Dart_TypedDataReleaseData(list_arg);
    Dart_PropagateError(
        Dart_NewApiError("Unimplemented type in "
                         "DartEmbedder::GetTypedDataListArgument"));
    return;
  }
  // Copy payload.
  *out = reinterpret_cast<uint8_t*>(malloc(data_len));
  *out_len = data_len;
  memmove(*out, data, data_len);
  // Release data.
  Dart_TypedDataReleaseData(list_arg);
}

Dart_Handle DartEmbedder::MakeUint8TypedData(uint8_t* bytes, intptr_t length) {
  Dart_Handle result = Dart_NewTypedData(Dart_TypedData_kUint8, length);
  if (Dart_IsError(result)) {
    Dart_PropagateError(result);
  }
  Dart_Handle err = Dart_ListSetAsBytes(result, 0, bytes, length);
  if (Dart_IsError(err)) {
    Dart_PropagateError(err);
  }
  return result;
}

}  // namespace dart
}  // namespace mojo
