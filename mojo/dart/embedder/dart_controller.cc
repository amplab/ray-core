// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/rand_util.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/sys_info.h"
#include "dart/runtime/include/dart_api.h"
#include "dart/runtime/include/dart_native_api.h"
#include "mojo/dart/embedder/builtin.h"
#include "mojo/dart/embedder/dart_controller.h"
#include "mojo/dart/embedder/mojo_dart_state.h"
#include "mojo/dart/embedder/observatory_archive.h"
#include "mojo/dart/embedder/vmservice.h"
#include "mojo/message_pump/message_pump_mojo.h"
#include "mojo/public/c/system/handle.h"
#include "mojo/public/platform/dart/dart_handle_watcher.h"
#include "tonic/dart_converter.h"
#include "tonic/dart_debugger.h"
#include "tonic/dart_dependency_catcher.h"
#include "tonic/dart_error.h"
#include "tonic/dart_library_loader.h"
#include "tonic/dart_library_provider.h"
#include "tonic/dart_library_provider_files.h"
#include "tonic/dart_library_provider_network.h"
#include "tonic/dart_message_handler.h"
#include "tonic/dart_script_loader_sync.h"
#include "tonic/dart_vm.h"

namespace mojo {
namespace dart {

extern const uint8_t* vm_isolate_snapshot_buffer;
extern const uint8_t* isolate_snapshot_buffer;

static const char* kAsyncLibURL = "dart:async";
static const char* kInternalLibURL = "dart:_internal";
static const char* kIsolateLibURL = "dart:isolate";
static const char* kCoreLibURL = "dart:core";

static uint8_t snapshot_magic_number[] = { 0xf5, 0xf5, 0xdc, 0xdc };

static Dart_Handle PrepareBuiltinLibraries(const std::string& package_root,
                                           const std::string& base_uri,
                                           const std::string& script_uri) {
  // First ensure all required libraries are available.
  Dart_Handle builtin_lib = Builtin::PrepareLibrary(Builtin::kBuiltinLibrary);
  Builtin::PrepareLibrary(Builtin::kMojoInternalLibrary);
  Builtin::PrepareLibrary(Builtin::kDartMojoIoLibrary);
  Dart_Handle url = Dart_NewStringFromCString(kInternalLibURL);
  DART_CHECK_VALID(url);
  Dart_Handle internal_lib = Dart_LookupLibrary(url);
  DART_CHECK_VALID(internal_lib);
  url = Dart_NewStringFromCString(kCoreLibURL);
  DART_CHECK_VALID(url);
  Dart_Handle core_lib = Dart_LookupLibrary(url);
  DART_CHECK_VALID(internal_lib);
  url = Dart_NewStringFromCString(kAsyncLibURL);
  DART_CHECK_VALID(url);
  Dart_Handle async_lib = Dart_LookupLibrary(url);
  DART_CHECK_VALID(async_lib);
  url = Dart_NewStringFromCString(kIsolateLibURL);
  DART_CHECK_VALID(url);
  Dart_Handle isolate_lib = Dart_LookupLibrary(url);
  DART_CHECK_VALID(isolate_lib);
  Dart_Handle mojo_internal_lib =
      Builtin::GetLibrary(Builtin::kMojoInternalLibrary);
  DART_CHECK_VALID(mojo_internal_lib);

  // We need to ensure that all the scripts loaded so far are finalized
  // as we are about to invoke some Dart code below to setup closures.
  Dart_Handle result = Dart_FinalizeLoading(false);
  DART_CHECK_VALID(result);

  // Import dart:_internal into dart:mojo.builtin for setting up hooks.
  result = Dart_LibraryImportLibrary(builtin_lib, internal_lib, Dart_Null());
  DART_CHECK_VALID(result);

  // Setup the internal library's 'internalPrint' function.
  Dart_Handle print = Dart_Invoke(builtin_lib,
                                  Dart_NewStringFromCString("_getPrintClosure"),
                                  0,
                                  nullptr);
  DART_CHECK_VALID(print);
  result = Dart_SetField(internal_lib,
                         Dart_NewStringFromCString("_printClosure"),
                         print);
  DART_CHECK_VALID(result);

  // Setup the 'scheduleImmediate' closure.
  Dart_Handle schedule_immediate_closure = Dart_Invoke(
      isolate_lib,
      Dart_NewStringFromCString("_getIsolateScheduleImmediateClosure"),
      0,
      nullptr);
  Dart_Handle schedule_args[1];
  schedule_args[0] = schedule_immediate_closure;
  result = Dart_Invoke(
      async_lib,
      Dart_NewStringFromCString("_setScheduleImmediateClosure"),
      1,
      schedule_args);
  DART_CHECK_VALID(result);

  // Set the script location.
  Dart_Handle uri = Dart_NewStringFromUTF8(
      reinterpret_cast<const uint8_t*>(script_uri.c_str()),
      script_uri.length());
  DART_CHECK_VALID(uri);
  result = Dart_SetField(builtin_lib,
                         Dart_NewStringFromCString("_rawScript"),
                         uri);
  DART_CHECK_VALID(result);

  // Set the base URI.
  uri = Dart_NewStringFromUTF8(
      reinterpret_cast<const uint8_t*>(base_uri.c_str()),
      base_uri.length());
  DART_CHECK_VALID(uri);
  result = Dart_SetField(builtin_lib,
                         Dart_NewStringFromCString("_rawUriBase"),
                         uri);
  DART_CHECK_VALID(result);

  // Setup the uriBase with the base uri of the mojo app.
  Dart_Handle uri_base = Dart_Invoke(
      builtin_lib,
      Dart_NewStringFromCString("_getUriBaseClosure"),
      0,
      nullptr);
  DART_CHECK_VALID(uri_base);
  result = Dart_SetField(core_lib,
                         Dart_NewStringFromCString("_uriBaseClosure"),
                         uri_base);
  DART_CHECK_VALID(result);

  DART_CHECK_VALID(Dart_Invoke(
      builtin_lib, Dart_NewStringFromCString("_setupHooks"), 0, nullptr));
  DART_CHECK_VALID(Dart_Invoke(
      isolate_lib, Dart_NewStringFromCString("_setupHooks"), 0, nullptr));

  return result;
}

static const intptr_t kStartIsolateArgumentsLength = 7;

static void SetupStartIsolateArguments(
    const DartControllerConfig& config,
    Dart_Handle main_closure,
    Dart_Handle* start_isolate_args) {
  // start_isolate_args:
  // [0] -> null
  // [1] -> main closure
  // [2] -> args list, list of script arguments in config,
  // [3] -> mojo handle,
  // [4] -> true,  // isSpawnUri
  // [5] -> null,
  // [6] -> null,
  start_isolate_args[0] = Dart_Null();
  DART_CHECK_VALID(start_isolate_args[0]);
  start_isolate_args[1] = main_closure;     // entryPoint
  DART_CHECK_VALID(start_isolate_args[1]);
  Dart_Handle script_args = Dart_NewList(config.script_flags_count);
  DART_CHECK_VALID(script_args);
  start_isolate_args[2] = script_args;
  DART_CHECK_VALID(start_isolate_args[2]);
  for (intptr_t i = 0; i < config.script_flags_count; i++) {
    Dart_ListSetAt(script_args, i,
                   Dart_NewStringFromCString(config.script_flags[i]));
  }
  start_isolate_args[3] = Dart_NewInteger(config.handle);
  DART_CHECK_VALID(start_isolate_args[3]);
  start_isolate_args[4] = Dart_True();
  DART_CHECK_VALID(start_isolate_args[4]);
  start_isolate_args[5] = Dart_Null();
  DART_CHECK_VALID(start_isolate_args[5]);
  start_isolate_args[6] = Dart_Null();
  DART_CHECK_VALID(start_isolate_args[6]);
}

static void CloseHandlesOnError(Dart_Handle error) {
  if (!Dart_IsError(error)) {
    return;
  }

  Dart_Handle mojo_core_lib =
      Builtin::GetLibrary(Builtin::kMojoInternalLibrary);
  DART_CHECK_VALID(mojo_core_lib);
  Dart_Handle handle_natives_type =
      Dart_GetType(mojo_core_lib,
                   Dart_NewStringFromCString("MojoHandleNatives"), 0, nullptr);
  DART_CHECK_VALID(handle_natives_type);
  Dart_Handle method_name = Dart_NewStringFromCString("_closeOpenHandles");
  CHECK(!Dart_IsError(method_name));
  Dart_Handle result =
      Dart_Invoke(handle_natives_type, method_name, 0, nullptr);
  DART_CHECK_VALID(result);

  auto isolate_data = MojoDartState::Current();
  if (!isolate_data->callbacks().exception.is_null()) {
    int64_t handles_closed = 0;
    Dart_Handle int_result = Dart_IntegerToInt64(result, &handles_closed);
    DART_CHECK_VALID(int_result);
    isolate_data->callbacks().exception.Run(error, handles_closed);
  }
}

static void SetupIsolate(Dart_Isolate isolate,
                         const DartControllerConfig& config) {
  tonic::DartIsolateScope isolate_scope(isolate);
  tonic::DartApiScope api_scope;

  Dart_Handle result;

  // Load the root library into the builtin library so that main can be found.
  Dart_Handle builtin_lib =
      Builtin::GetLibrary(Builtin::kBuiltinLibrary);
  DART_CHECK_VALID(builtin_lib);
  Dart_Handle root_lib = Dart_RootLibrary();
  DART_CHECK_VALID(root_lib);
  result = Dart_LibraryImportLibrary(builtin_lib, root_lib, Dart_Null());
  DART_CHECK_VALID(result);

  if (config.compile_all) {
    result = Dart_CompileAll();
    DART_CHECK_VALID(result);
  }

  Dart_Handle main_closure = Dart_Invoke(
      builtin_lib,
      Dart_NewStringFromCString("_getMainClosure"),
      0,
      nullptr);
  DART_CHECK_VALID(main_closure);

  Dart_Handle start_isolate_args[kStartIsolateArgumentsLength];
  SetupStartIsolateArguments(config, main_closure, &start_isolate_args[0]);
  Dart_Handle isolate_lib =
      Dart_LookupLibrary(Dart_NewStringFromCString(kIsolateLibURL));
  DART_CHECK_VALID(isolate_lib);

  result = Dart_Invoke(isolate_lib,
                       Dart_NewStringFromCString("_startIsolate"),
                       kStartIsolateArgumentsLength,
                       start_isolate_args);
  DART_CHECK_VALID(result);
}

static bool RunIsolate(Dart_Isolate isolate) {
  tonic::DartIsolateScope isolate_scope(isolate);
  tonic::DartApiScope api_scope;

  Dart_Handle result = Dart_RunLoop();
  CloseHandlesOnError(result);

  // Here we log the error, but we don't do DART_CHECK_VALID because we don't
  // want to bring the whole process down due to an error in application code,
  // whereas above we do want to bring the whole process down for a bug in
  // library or generated code.
  return tonic::LogIfError(result);
}

Dart_Handle DartController::LibraryTagHandler(Dart_LibraryTag tag,
                                              Dart_Handle library,
                                              Dart_Handle url) {
  if (tag == Dart_kCanonicalizeUrl) {
    std::string string = tonic::StdStringFromDart(url);
    if (base::StartsWith(string, "dart:", base::CompareCase::SENSITIVE))
      return url;
  }
  return tonic::DartLibraryLoader::HandleLibraryTag(tag, library, url);
}

Dart_Isolate DartController::CreateIsolateHelper(
    void* dart_app,
    Dart_IsolateFlags* flags,
    const DartControllerConfig& config) {
  auto isolate_data = new MojoDartState(dart_app,
                                        config.callbacks,
                                        config.script_uri,
                                        config.base_uri,
                                        config.package_root,
                                        config.use_network_loader,
                                        config.use_dart_run_loop);
  CHECK(isolate_snapshot_buffer != nullptr);
  Dart_Isolate isolate =
      Dart_CreateIsolate(config.base_uri.c_str(),
                         "main",
                         isolate_snapshot_buffer,
                         flags,
                         isolate_data,
                         config.error);
  if (isolate == nullptr) {
    delete isolate_data;
    return nullptr;
  }
  if (config.override_pause_isolate_flags) {
    Dart_SetShouldPauseOnStart(config.pause_isolate_on_start);
    Dart_SetShouldPauseOnExit(config.pause_isolate_on_exit);
  }
  Dart_ExitIsolate();

  isolate_data->SetIsolate(isolate);
  if (service_connector_ != nullptr) {
    // This is not supported in the unit test harness.
    isolate_data->BindNetworkService(
        service_connector_->ConnectToService(
            DartControllerServiceConnector::kNetworkServiceId));
  }

  // Setup isolate and load script.
  {
    tonic::DartIsolateScope isolate_scope(isolate);
    tonic::DartApiScope api_scope;
    // Setup loader.
    const char* package_root_str = nullptr;
    if (config.package_root.empty()) {
      package_root_str = "/";
    } else {
      package_root_str = config.package_root.c_str();
    }
    isolate_data->library_loader().set_magic_number(
        snapshot_magic_number, sizeof(snapshot_magic_number));
    if (config.use_network_loader) {
      mojo::NetworkService* network_service = isolate_data->network_service();
      isolate_data->set_library_provider(
          new tonic::DartLibraryProviderNetwork(network_service));
    } else {
      isolate_data->set_library_provider(
          new tonic::DartLibraryProviderFiles(
              base::FilePath(package_root_str)));
    }
    Dart_Handle result = Dart_SetLibraryTagHandler(LibraryTagHandler);
    DART_CHECK_VALID(result);
    // Prepare builtin and its dependent libraries.
    result = PrepareBuiltinLibraries(config.package_root,
                                     config.base_uri,
                                     config.script_uri);
    DART_CHECK_VALID(result);

    // Set the handle watcher's control handle in the spawning isolate.
    result = SetHandleWatcherControlHandle();
    DART_CHECK_VALID(result);

    if (!config.use_dart_run_loop) {
      // Verify that we are being created on a thread with a message loop.
      DCHECK(base::MessageLoop::current());
      // Install the message handler.
      isolate_data->message_handler().Initialize(
          base::MessageLoop::current()->task_runner());
    }

    // The VM is creating the service isolate.
    if (Dart_IsServiceIsolate(isolate)) {
      service_isolate_spawned_ = true;
      const intptr_t port =
          (SupportDartMojoIo() && enable_observatory_) ? 0 : -1;
      InitializeDartMojoIo("");
      if (!VmService::Setup("127.0.0.1", port)) {
        *config.error = strdup(VmService::GetErrorMessage());
        return nullptr;
      }
      return isolate;
    }

    tonic::DartScriptLoaderSync::LoadScript(
        config.script_uri,
        isolate_data->library_provider());
    InitializeDartMojoIo(config.base_uri);
  }

  if (isolate_data->library_loader().error_during_loading()) {
    *config.error =
        strdup("Library loader reported error during loading. See log.");
    Dart_EnterIsolate(isolate);
    Dart_ShutdownIsolate();
    return nullptr;
  }

  // Make the isolate runnable so that it is ready to handle messages.
  bool retval = Dart_IsolateMakeRunnable(isolate);
  if (!retval) {
    *config.error =
        strdup("Invalid isolate state - Unable to make it runnable");
    Dart_EnterIsolate(isolate);
    Dart_ShutdownIsolate();
    return nullptr;
  }

  DCHECK(Dart_CurrentIsolate() == nullptr);

  return isolate;
}


Dart_Handle DartController::SetHandleWatcherControlHandle() {
  CHECK(handle_watcher_producer_handle_ != MOJO_HANDLE_INVALID);
  Dart_Handle mojo_internal_lib =
      Builtin::GetLibrary(Builtin::kMojoInternalLibrary);
  DART_CHECK_VALID(mojo_internal_lib);
  Dart_Handle handle_watcher_type = Dart_GetType(
      mojo_internal_lib,
      Dart_NewStringFromCString("MojoHandleWatcher"),
      0,
      nullptr);
  DART_CHECK_VALID(handle_watcher_type);
  Dart_Handle field_name = Dart_NewStringFromCString("mojoControlHandle");
  DART_CHECK_VALID(field_name);
  Dart_Handle control_port_value =
      Dart_NewInteger(handle_watcher_producer_handle_);
  Dart_Handle result =
      Dart_SetField(handle_watcher_type, field_name, control_port_value);
  return result;
}

Dart_Isolate DartController::IsolateCreateCallback(const char* script_uri,
                                                   const char* main,
                                                   const char* package_root,
                                                   const char* package_config,
                                                   Dart_IsolateFlags* flags,
                                                   void* callback_data,
                                                   char** error) {
  DCHECK(script_uri != nullptr);
  auto parent_isolate_data = MojoDartState::Cast(callback_data);

  DartControllerConfig config;
  config.script_uri = script_uri;
  // If it's a file URI, strip the scheme.
  const char* file_scheme = "file://";
  if (base::StartsWith(config.script_uri,
                       file_scheme,
                       base::CompareCase::SENSITIVE)) {
    config.script_uri = config.script_uri.substr(strlen(file_scheme));
  }

  // Being spawned from an existing isolate?
  const bool has_parent_isolate = (parent_isolate_data != nullptr);
  const bool is_spawn_function = has_parent_isolate &&
      (parent_isolate_data->script_uri() == config.script_uri);

  if (is_spawn_function) {
    // We are spawning a function, use the parent isolate's base URI.
    config.base_uri = parent_isolate_data->base_uri();
  } else {
    // If we are spawning a URI directly, use the URI as the base URI.
    config.base_uri = config.script_uri;
  }

  if (package_root == nullptr) {
    if (has_parent_isolate) {
      config.package_root = parent_isolate_data->package_root();
    }
  } else {
    config.package_root = std::string(package_root);
  }

  if (has_parent_isolate) {
    config.application_data = parent_isolate_data->application_data();
    config.use_network_loader = parent_isolate_data->use_network_loader();
    config.callbacks = parent_isolate_data->callbacks();
  }

  return CreateIsolateHelper(config.application_data,
                             flags,
                             config);
}

void DartController::IsolateShutdownCallback(void* callback_data) {
  {
    tonic::DartApiScope api_scope;

    // Shut down dart:io.
    ShutdownDartMojoIo();
  }

  auto isolate_data = MojoDartState::Cast(callback_data);
  delete isolate_data;
}


bool DartController::initialized_ = false;
MojoHandle DartController::handle_watcher_producer_handle_ =
    MOJO_HANDLE_INVALID;
bool DartController::service_isolate_running_ = false;
bool DartController::service_isolate_spawned_ = false;
bool DartController::strict_compilation_ = false;
bool DartController::enable_observatory_ = true;
DartControllerServiceConnector* DartController::service_connector_ = nullptr;
base::Lock DartController::lock_;

bool DartController::SupportDartMojoIo() {
  return service_connector_ != nullptr;
}

void DartController::InitializeDartMojoIo(const std::string& base_uri) {
  Dart_Isolate current_isolate = Dart_CurrentIsolate();
  CHECK(current_isolate != nullptr);
  if (!SupportDartMojoIo()) {
    return;
  }
  CHECK(service_connector_ != nullptr);
  // Get handles to the network and files services.
  MojoHandle network_service_mojo_handle = MOJO_HANDLE_INVALID;
  network_service_mojo_handle =
        service_connector_->ConnectToService(
            DartControllerServiceConnector::kNetworkServiceId);
  MojoHandle files_service_mojo_handle = MOJO_HANDLE_INVALID;
  files_service_mojo_handle =
      service_connector_->ConnectToService(
          DartControllerServiceConnector::kFilesServiceId);
  if ((network_service_mojo_handle == MOJO_HANDLE_INVALID) &&
      (files_service_mojo_handle == MOJO_HANDLE_INVALID)) {
    // Not supported.
    return;
  }
  // Pass handles into 'dart:io' library.
  Dart_Handle mojo_io_library =
      Builtin::GetLibrary(Builtin::kDartMojoIoLibrary);
  CHECK(!Dart_IsError(mojo_io_library));
  Dart_Handle method_name = Dart_NewStringFromCString("_initialize");
  CHECK(!Dart_IsError(method_name));
  Dart_Handle network_service_handle =
      Dart_NewInteger(network_service_mojo_handle);
  CHECK(!Dart_IsError(network_service_handle));
  Dart_Handle files_service_handle =
      Dart_NewInteger(files_service_mojo_handle);
  CHECK(!Dart_IsError(files_service_handle));
  Dart_Handle script_path = Dart_NewStringFromUTF8(
      reinterpret_cast<const uint8_t*>(base_uri.data()),
      base_uri.length());
  Dart_Handle arguments[] = {
    network_service_handle,
    files_service_handle,
    script_path,
  };
  Dart_Handle result = Dart_Invoke(mojo_io_library,
                                   method_name,
                                   3,
                                   &arguments[0]);
  CHECK(!Dart_IsError(result));
}

void DartController::ShutdownDartMojoIo() {
  Dart_Isolate current_isolate = Dart_CurrentIsolate();
  CHECK(current_isolate != nullptr);
  if (!SupportDartMojoIo()) {
    return;
  }
  Dart_Handle mojo_io_library =
      Builtin::GetLibrary(Builtin::kDartMojoIoLibrary);
  CHECK(!Dart_IsError(mojo_io_library));
  Dart_Handle method_name = Dart_NewStringFromCString("_shutdown");
  CHECK(!Dart_IsError(method_name));
  Dart_Handle result = Dart_Invoke(mojo_io_library,
                                   method_name,
                                   0,
                                   nullptr);
  if (Dart_IsError(result)) {
    Dart_PropagateError(result);
  }
}


static Dart_Handle MakeUint8Array(const uint8_t* buffer,
                                  unsigned int buffer_len) {
  const intptr_t len = static_cast<intptr_t>(buffer_len);
  Dart_Handle array = Dart_NewTypedData(Dart_TypedData_kUint8, len);
  DART_CHECK_VALID(array);
  {
    Dart_TypedData_Type td_type;
    void* td_data = nullptr;
    intptr_t td_len = 0;
    Dart_Handle result =
        Dart_TypedDataAcquireData(array, &td_type, &td_data, &td_len);
    DART_CHECK_VALID(result);
    CHECK_EQ(td_type, Dart_TypedData_kUint8);
    CHECK(td_data != nullptr);
    CHECK_EQ(td_len, len);
    memmove(td_data, buffer, td_len);
    result = Dart_TypedDataReleaseData(array);
    DART_CHECK_VALID(result);
  }
  return array;
}

static Dart_Handle GetVMServiceAssetsArchiveCallback() {
  return MakeUint8Array(
      ::dart::observatory::observatory_assets_archive,
      ::dart::observatory::observatory_assets_archive_len);
}

void DartController::InitVmIfNeeded(Dart_EntropySource entropy,
                                    bool enable_dart_timeline,
                                    const char** vm_flags,
                                    int vm_flags_count) {
  base::AutoLock al(lock_);
  if (initialized_) {
    return;
  }

  // Start a handle watcher.
  handle_watcher_producer_handle_ = HandleWatcher::Start();

  std::vector<const char*> flags;
  // Disable access dart:mirrors library.
  flags.push_back("--enable_mirrors=false");
  // Force await and async to be keywords even outside of an async function.
  flags.push_back("--await_is_keyword");
  // Enable background compilation
  flags.push_back("--background_compilation=true");
  // Disable code write protection
  // TODO(johnmccutchan): This might be a security issue once Mojo gets a
  // security sandbox. Revisit when that happens.
  flags.push_back("--write_protect_code=false");
  // Add remaining flags.
  for (int i = 0; i < vm_flags_count; ++i) {
    flags.push_back(vm_flags[i]);
  }

  tonic::DartVM::Config config;
  config.vm_isolate_snapshot = vm_isolate_snapshot_buffer;
  config.create = IsolateCreateCallback;
  config.shutdown = IsolateShutdownCallback;
  config.entropy_source = entropy;
  config.get_service_assets = GetVMServiceAssetsArchiveCallback;

  CHECK(tonic::DartVM::Initialize(config, flags));

  initialized_ = true;
  if (enable_dart_timeline) {
    Dart_GlobalTimelineSetRecordedStreams(DART_TIMELINE_STREAM_DART);
  }
}

void DartController::BlockForServiceIsolate() {
  base::AutoLock al(lock_);
  BlockForServiceIsolateLocked();
}

void DartController::BlockForServiceIsolateLocked() {
  if (service_isolate_running_) {
    return;
  }
  // By waiting for the load port, we ensure that the service isolate is fully
  // running before returning.
  Dart_ServiceWaitForLoadPort();
  service_isolate_running_ = true;
}

static bool GenerateEntropy(uint8_t* buffer, intptr_t length) {
  base::RandBytes(reinterpret_cast<void*>(buffer), length);
  return true;
}

bool DartController::Initialize(
    DartControllerServiceConnector* service_connector,
    bool strict_compilation,
    bool enable_observatory,
    bool enable_dart_timeline,
    const char** vm_flags,
    int vm_flags_count) {
  service_connector_ = service_connector;
  enable_observatory_ = enable_observatory;
  strict_compilation_ = strict_compilation;
  InitVmIfNeeded(GenerateEntropy,
                 enable_dart_timeline,
                 vm_flags,
                 vm_flags_count);
  return true;
}

bool DartController::RunToCompletion(Dart_Isolate isolate) {
  return !RunIsolate(isolate);
}

void DartController::SetIsolateFlags(Dart_IsolateFlags* flags,
                                     bool strict_compilation) {
  flags->version = DART_FLAGS_CURRENT_VERSION;
  flags->enable_type_checks = strict_compilation;
  flags->enable_asserts = strict_compilation;
  flags->enable_error_on_bad_type = strict_compilation;
  flags->enable_error_on_bad_override = strict_compilation;
}

Dart_Isolate DartController::StartupIsolate(
    const DartControllerConfig& config) {
  const bool strict = strict_compilation_ || config.strict_compilation;

  Dart_IsolateFlags flags;
  SetIsolateFlags(&flags, strict);

  Dart_Isolate isolate = CreateIsolateHelper(config.application_data,
                                             &flags,
                                             config);

  if (isolate == nullptr) {
    return isolate;
  }

  SetupIsolate(isolate, config);

  return isolate;
}

void DartController::ShutdownIsolate(Dart_Isolate isolate) {
  Dart_EnterIsolate(isolate);
  Dart_ShutdownIsolate();
}

void DartController::Shutdown() {
  base::AutoLock al(lock_);
  if (!initialized_) {
    return;
  }
  BlockForServiceIsolateLocked();
  HandleWatcher::StopAll();
  CHECK(tonic::DartVM::Cleanup());
  service_isolate_running_ = false;
  initialized_ = false;
}

}  // namespace apps
}  // namespace mojo
