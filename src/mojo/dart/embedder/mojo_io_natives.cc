// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <string.h>
#include <limits>

#include "dart/runtime/include/dart_api.h"
#include "mojo/dart/embedder/builtin.h"
#include "mojo/dart/embedder/common.h"
#include "mojo/dart/embedder/io/filter.h"
#include "mojo/dart/embedder/io/internet_address.h"
#include "mojo/dart/embedder/mojo_dart_state.h"
#include "mojo/public/cpp/system/macros.h"

namespace mojo {
namespace dart {

#define MOJO_IO_NATIVE_LIST(V)                                                 \
  V(Filter_CreateZLibDeflate, 8)                                               \
  V(Filter_CreateZLibInflate, 4)                                               \
  V(Filter_Process, 4)                                                         \
  V(Filter_Processed, 3)                                                       \
  V(InternetAddress_Parse, 1)                                                  \
  V(InternetAddress_Reverse, 1)                                                \
  V(Platform_NumberOfProcessors, 0)                                            \
  V(Platform_OperatingSystem, 0)                                               \
  V(Platform_PathSeparator, 0)                                                 \
  V(Platform_LocalHostname, 0)                                                 \
  V(Platform_ExecutableName, 0)                                                \
  V(Platform_Environment, 0)                                                   \
  V(Platform_ExecutableArguments, 0)                                           \
  V(Platform_PackageRoot, 0)                                                   \
  V(Platform_GetVersion, 0)                                                    \
  V(Process_Pid, 0)                                                            \

MOJO_IO_NATIVE_LIST(DECLARE_FUNCTION);

static struct NativeEntries {
  const char* name;
  Dart_NativeFunction function;
  int argument_count;
} MojoEntries[] = {MOJO_IO_NATIVE_LIST(REGISTER_FUNCTION)};

Dart_NativeFunction MojoIoNativeLookup(Dart_Handle name,
                                       int argument_count,
                                       bool* auto_setup_scope) {
  const char* function_name = nullptr;
  Dart_Handle result = Dart_StringToCString(name, &function_name);
  DART_CHECK_VALID(result);
  assert(function_name != nullptr);
  assert(auto_setup_scope != nullptr);
  *auto_setup_scope = true;
  size_t num_entries = MOJO_ARRAYSIZE(MojoEntries);
  for (size_t i = 0; i < num_entries; ++i) {
    const struct NativeEntries& entry = MojoEntries[i];
    if (!strcmp(function_name, entry.name) &&
        (entry.argument_count == argument_count)) {
      return entry.function;
    }
  }
  return nullptr;
}

const uint8_t* MojoIoNativeSymbol(Dart_NativeFunction nf) {
  size_t num_entries = MOJO_ARRAYSIZE(MojoEntries);
  for (size_t i = 0; i < num_entries; ++i) {
    const struct NativeEntries& entry = MojoEntries[i];
    if (entry.function == nf) {
      return reinterpret_cast<const uint8_t*>(entry.name);
    }
  }
  return nullptr;
}

class IOBuffer {
 public:
  static Dart_Handle Allocate(intptr_t size, uint8_t **buffer) {
    uint8_t* data = Allocate(size);
    Dart_Handle result = Dart_NewExternalTypedData(
        Dart_TypedData_kUint8, data, size);
    Dart_NewWeakPersistentHandle(result, data, size, IOBuffer::Finalizer);

    if (Dart_IsError(result)) {
      Free(data);
      Dart_PropagateError(result);
    }
    if (buffer != NULL) {
      *buffer = data;
    }
    return result;
  }

  // Allocate IO buffer storage.
  static uint8_t* Allocate(intptr_t size) {
    return new uint8_t[size];
  }

  // Function for disposing of IO buffer storage. All backing storage
  // for IO buffers must be freed using this function.
  static void Free(void* buffer) {
    delete[] reinterpret_cast<uint8_t*>(buffer);
  }

  // Function for finalizing external byte arrays used as IO buffers.
  static void Finalizer(void* isolate_callback_data,
                        Dart_WeakPersistentHandle handle,
                        void* buffer) {
    Free(buffer);
  }

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(IOBuffer);
};

static Dart_Handle GetFilter(Dart_Handle filter_obj, Filter** filter) {
  CHECK(filter != NULL);
  Filter* result;
  Dart_Handle err = Filter::GetFilterNativeField(filter_obj, &result);
  if (Dart_IsError(err)) {
    return err;
  }
  if (result == NULL) {
    return Dart_NewApiError("Filter was destroyed");
  }

  *filter = result;
  return Dart_Null();
}

static Dart_Handle CopyDictionary(Dart_Handle dictionary_obj,
                                  uint8_t** dictionary) {
  CHECK(dictionary != NULL);
  uint8_t* src = NULL;
  intptr_t size;
  Dart_TypedData_Type type;

  Dart_Handle err = Dart_ListLength(dictionary_obj, &size);
  if (Dart_IsError(err)) {
    return err;
  }

  uint8_t* result = new uint8_t[size];
  if (result == NULL) {
    return Dart_NewApiError("Could not allocate new dictionary");
  }

  err = Dart_TypedDataAcquireData(
      dictionary_obj, &type, reinterpret_cast<void**>(&src), &size);
  if (!Dart_IsError(err)) {
    memmove(result, src, size);
    Dart_TypedDataReleaseData(dictionary_obj);
  } else {
    err = Dart_ListGetAsBytes(dictionary_obj, 0, result, size);
    if (Dart_IsError(err)) {
      delete[] result;
      return err;
    }
  }

  *dictionary = result;
  return Dart_Null();
}

void Filter_CreateZLibInflate(Dart_NativeArguments args) {
  Dart_Handle filter_obj = Dart_GetNativeArgument(args, 0);
  int64_t window_bits = DartEmbedder::GetIntegerArgument(args, 1);
  Dart_Handle dict_obj = Dart_GetNativeArgument(args, 2);
  bool raw = DartEmbedder::GetBooleanArgument(args, 3);

  Dart_Handle err;
  uint8_t* dictionary = NULL;
  intptr_t dictionary_length = 0;
  if (!Dart_IsNull(dict_obj)) {
    err = CopyDictionary(dict_obj, &dictionary);
    if (Dart_IsError(err)) {
      Dart_PropagateError(err);
    }
    CHECK(dictionary != NULL);
    dictionary_length = 0;
    err = Dart_ListLength(dict_obj, &dictionary_length);
    if (Dart_IsError(err)) {
      delete[] dictionary;
      Dart_PropagateError(err);
    }
  }

  ZLibInflateFilter* filter = new ZLibInflateFilter(
      static_cast<int32_t>(window_bits), dictionary, dictionary_length, raw);
  if (filter == NULL) {
    delete[] dictionary;
    Dart_PropagateError(Dart_NewApiError(
        "Could not allocate ZLibInflateFilter"));
  }
  if (!filter->Init()) {
    delete filter;
    Dart_ThrowException(DartEmbedder::NewInternalError(
        "Failed to create ZLibInflateFilter"));
  }
  err = Filter::SetFilterAndCreateFinalizer(
      filter_obj, filter, sizeof(*filter) + dictionary_length);
  if (Dart_IsError(err)) {
    delete filter;
    Dart_PropagateError(err);
  }
}

void Filter_CreateZLibDeflate(Dart_NativeArguments args) {
  Dart_Handle filter_obj = Dart_GetNativeArgument(args, 0);
  bool gzip = DartEmbedder::GetBooleanArgument(args, 1);
  Dart_Handle level_obj = Dart_GetNativeArgument(args, 2);
  int64_t level = DartEmbedder::GetInt64ValueCheckRange(
      level_obj,
      std::numeric_limits<int32_t>::min(),
      std::numeric_limits<int32_t>::max());
  int64_t window_bits = DartEmbedder::GetIntegerArgument(args, 3);
  int64_t mem_level = DartEmbedder::GetIntegerArgument(args, 4);
  int64_t strategy = DartEmbedder::GetIntegerArgument(args, 5);
  Dart_Handle dict_obj = Dart_GetNativeArgument(args, 6);
  bool raw = DartEmbedder::GetBooleanArgument(args, 7);

  Dart_Handle err;
  uint8_t* dictionary = NULL;
  intptr_t dictionary_length = 0;
  if (!Dart_IsNull(dict_obj)) {
    err = CopyDictionary(dict_obj, &dictionary);
    if (Dart_IsError(err)) {
      Dart_PropagateError(err);
    }
    CHECK(dictionary != NULL);
    dictionary_length = 0;
    err = Dart_ListLength(dict_obj, &dictionary_length);
    if (Dart_IsError(err)) {
      delete[] dictionary;
      Dart_PropagateError(err);
    }
  }

  ZLibDeflateFilter* filter = new ZLibDeflateFilter(
      gzip,
      static_cast<int32_t>(level),
      static_cast<int32_t>(window_bits),
      static_cast<int32_t>(mem_level),
      static_cast<int32_t>(strategy),
      dictionary, dictionary_length, raw);
  if (filter == NULL) {
    delete[] dictionary;
    Dart_PropagateError(Dart_NewApiError(
        "Could not allocate ZLibDeflateFilter"));
  }
  if (!filter->Init()) {
    delete filter;
    Dart_ThrowException(DartEmbedder::NewInternalError(
        "Failed to create ZLibDeflateFilter"));
  }
  Dart_Handle result = Filter::SetFilterAndCreateFinalizer(
      filter_obj, filter, sizeof(*filter) + dictionary_length);
  if (Dart_IsError(result)) {
    delete filter;
    Dart_PropagateError(result);
  }
}

void Filter_Process(Dart_NativeArguments args) {
  Dart_Handle filter_obj = Dart_GetNativeArgument(args, 0);
  Dart_Handle data_obj = Dart_GetNativeArgument(args, 1);
  intptr_t start = DartEmbedder::GetIntptrArgument(args, 2);
  intptr_t end = DartEmbedder::GetIntptrArgument(args, 3);
  intptr_t chunk_length = end - start;
  intptr_t length;
  Dart_TypedData_Type type;
  uint8_t* buffer = NULL;

  Filter* filter = NULL;
  Dart_Handle err = GetFilter(filter_obj, &filter);
  if (Dart_IsError(err)) {
    Dart_PropagateError(err);
  }

  Dart_Handle result = Dart_TypedDataAcquireData(
      data_obj, &type, reinterpret_cast<void**>(&buffer), &length);
  if (!Dart_IsError(result)) {
    CHECK(type == Dart_TypedData_kUint8 || type == Dart_TypedData_kInt8);
    if (type != Dart_TypedData_kUint8 && type != Dart_TypedData_kInt8) {
      Dart_TypedDataReleaseData(data_obj);
      Dart_ThrowException(DartEmbedder::NewInternalError(
          "Invalid argument passed to Filter_Process"));
    }
    uint8_t* zlib_buffer = new uint8_t[chunk_length];
    if (zlib_buffer == NULL) {
      Dart_TypedDataReleaseData(data_obj);
      Dart_PropagateError(Dart_NewApiError("Could not allocate zlib buffer"));
    }

    memmove(zlib_buffer, buffer + start, chunk_length);
    Dart_TypedDataReleaseData(data_obj);
    buffer = zlib_buffer;
  } else {
    err = Dart_ListLength(data_obj, &length);
    if (Dart_IsError(err)) {
      Dart_PropagateError(err);
    }
    buffer = new uint8_t[chunk_length];
    if (buffer == NULL) {
      Dart_PropagateError(Dart_NewApiError("Could not allocate buffer"));
    }
    err = Dart_ListGetAsBytes(data_obj, start, buffer, chunk_length);
    if (Dart_IsError(err)) {
      delete[] buffer;
      Dart_PropagateError(err);
    }
  }
  // Process will take ownership of buffer, if successful.
  if (!filter->Process(buffer, chunk_length)) {
    delete[] buffer;
    Dart_ThrowException(DartEmbedder::NewInternalError(
        "Call to Process while still processing data"));
  }
}

void Filter_Processed(Dart_NativeArguments args) {
  Dart_Handle filter_obj = Dart_GetNativeArgument(args, 0);
  bool flush = DartEmbedder::GetBooleanArgument(args, 1);
  bool end = DartEmbedder::GetBooleanArgument(args, 2);

  Filter* filter = NULL;
  Dart_Handle err = GetFilter(filter_obj, &filter);
  if (Dart_IsError(err)) {
    Dart_PropagateError(err);
  }

  intptr_t read = filter->Processed(filter->processed_buffer(),
                                    filter->processed_buffer_size(),
                                    flush,
                                    end);
  if (read < 0) {
    Dart_ThrowException(DartEmbedder::NewInternalError(
        "Filter error, bad data"));
  } else if (read == 0) {
    Dart_SetReturnValue(args, Dart_Null());
  } else {
    uint8_t* io_buffer;
    Dart_Handle result = IOBuffer::Allocate(read, &io_buffer);
    memmove(io_buffer, filter->processed_buffer(), read);
    Dart_SetReturnValue(args, result);
  }
}

void InternetAddress_Parse(Dart_NativeArguments arguments) {
  const char* address = DartEmbedder::GetStringArgument(arguments, 0);
  CHECK(address != nullptr);
  RawAddr raw;
  int type = strchr(address, ':') == nullptr ? InternetAddress::TYPE_IPV4
                                          : InternetAddress::TYPE_IPV6;
  intptr_t length = (type == InternetAddress::TYPE_IPV4) ?
      IPV4_RAW_ADDR_LENGTH : IPV6_RAW_ADDR_LENGTH;
  if (InternetAddress::Parse(type, address, &raw)) {
    Dart_SetReturnValue(arguments,
                        DartEmbedder::MakeUint8TypedData(&raw.bytes[0],
                                                         length));
  } else {
    DartEmbedder::SetNullReturn(arguments);
  }
}

void InternetAddress_Reverse(Dart_NativeArguments arguments) {
  uint8_t* addr = nullptr;
  intptr_t addr_len = 0;
  DartEmbedder::GetTypedDataListArgument(arguments, 0, &addr, &addr_len);
  if (addr_len == 0) {
    DartEmbedder::SetNullReturn(arguments);
    return;
  }
  // IPv4 or IPv6 address length.
  CHECK((addr_len == 4) || (addr_len == 16));
  RawAddr raw_addr;
  for (intptr_t i = 0; i < addr_len; i++) {
    raw_addr.bytes[i] = addr[i];
  }
  free(addr);

  const intptr_t kMaxHostLength = 1025;
  char host[kMaxHostLength];
  intptr_t error_code = 0;
  const char* error_description = nullptr;
  bool success = InternetAddress::Reverse(raw_addr, addr_len,
                                          &host[0], kMaxHostLength,
                                          &error_code, &error_description);
  // List of length 2.
  // [0] -> code (0 indicates success).
  // [1] -> error or host.
  Dart_Handle result_list = Dart_NewList(2);
  Dart_ListSetAt(result_list, 0, Dart_NewInteger(error_code));
  if (success) {
    Dart_ListSetAt(result_list, 1, DartEmbedder::NewCString(host));
  } else {
    Dart_ListSetAt(result_list, 1, DartEmbedder::NewCString(error_description));
  }
  Dart_SetReturnValue(arguments, result_list);
}

void Platform_NumberOfProcessors(Dart_NativeArguments arguments) {
  // TODO(johnmccutchan): Is an implementation needed?
  DartEmbedder::SetNullReturn(arguments);
}

void Platform_OperatingSystem(Dart_NativeArguments arguments) {
  // TODO(johnmccutchan): Is an implementation needed?
  DartEmbedder::SetNullReturn(arguments);
}

void Platform_PathSeparator(Dart_NativeArguments arguments) {
  // TODO(johnmccutchan): Is an implementation needed?
  DartEmbedder::SetNullReturn(arguments);
}

void Platform_LocalHostname(Dart_NativeArguments arguments) {
  // TODO(johnmccutchan): Is an implementation needed?
  DartEmbedder::SetNullReturn(arguments);
}

void Platform_ExecutableName(Dart_NativeArguments arguments) {
  // TODO(johnmccutchan): Is an implementation needed?
  DartEmbedder::SetNullReturn(arguments);
}

void Platform_Environment(Dart_NativeArguments arguments) {
  // TODO(johnmccutchan): Is an implementation needed?
  Dart_SetReturnValue(arguments, Dart_NewList(0));
}

void Platform_ExecutableArguments(Dart_NativeArguments arguments) {
  // TODO(johnmccutchan): Is an implementation needed?
  DartEmbedder::SetNullReturn(arguments);
}

void Platform_PackageRoot(Dart_NativeArguments arguments) {
  auto isolate_data = MojoDartState::Current();
  assert(isolate_data != nullptr);
  Dart_SetReturnValue(
      arguments,
      Dart_NewStringFromCString(isolate_data->package_root().c_str()));
}

void Platform_GetVersion(Dart_NativeArguments arguments) {
  // TODO(johnmccutchan): Consider incorporating Mojo version.
  Dart_SetReturnValue(arguments,
                      Dart_NewStringFromCString(Dart_VersionString()));
}

void Process_Pid(Dart_NativeArguments arguments) {
  // TODO(rudominer) After sandboxing is implemented getpid() will not return
  // the real pid. Most likely it will return the value 1. We need to decide
  // what behavior we want Dart's pid getter to have when sandboxed.
  pid_t pid = getpid();
  Dart_SetIntegerReturnValue(arguments, static_cast<int64_t>(pid));
}

}  // namespace dart
}  // namespace mojo
