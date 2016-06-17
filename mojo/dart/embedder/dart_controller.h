// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_DART_EMBEDDER_DART_CONTROLLER_H_
#define MOJO_DART_EMBEDDER_DART_CONTROLLER_H_

#include <unordered_set>

#include "base/synchronization/lock.h"
#include "dart/runtime/include/dart_api.h"
#include "mojo/dart/embedder/mojo_dart_state.h"
#include "mojo/public/c/system/handle.h"

namespace tonic {
class DartDependency;
class DartLibraryLoader;
}

namespace mojo {
namespace dart {

struct DartControllerConfig {
  static const bool kDefaultUseNetworkLoader = false;
  static const bool kDefaultUseDartRunLoop = true;
  static const bool kDefaultStrictCompilation = false;
  static const bool kDefaultPauseOnStart = false;
  static const bool kDefaultPauseOnExit = false;

  DartControllerConfig()
      : application_data(nullptr),
        strict_compilation(kDefaultStrictCompilation),
        entropy(nullptr),
        vm_flags(nullptr),
        vm_flags_count(0),
        script_flags(nullptr),
        script_flags_count(0),
        handle(MOJO_HANDLE_INVALID),
        compile_all(false),
        error(nullptr),
        use_network_loader(kDefaultUseNetworkLoader),
        use_dart_run_loop(kDefaultUseDartRunLoop),
        override_pause_isolate_flags(false),
        pause_isolate_on_start(kDefaultPauseOnStart),
        pause_isolate_on_exit(kDefaultPauseOnExit) {
  }

  void SetVmFlags(const char** vm_flags, intptr_t vm_flags_count) {
    this->vm_flags = vm_flags;
    this->vm_flags_count = vm_flags_count;
  }

  void SetScriptFlags(const char** script_flags, intptr_t script_flags_count) {
    this->script_flags = script_flags;
    this->script_flags_count = script_flags_count;
  }

  void OverridePauseIsolateFlags(bool pause_on_start, bool pause_on_exit) {
    override_pause_isolate_flags = true;
    pause_isolate_on_start = pause_on_start;
    pause_isolate_on_exit = pause_on_exit;
  }

  void* application_data;
  bool strict_compilation;
  std::string script_uri;
  std::string package_root;
  std::string base_uri;
  IsolateCallbacks callbacks;
  Dart_EntropySource entropy;
  const char** vm_flags;
  int vm_flags_count;
  const char** script_flags;
  int script_flags_count;
  MojoHandle handle;
  bool compile_all;
  char** error;
  bool use_network_loader;
  bool use_dart_run_loop;
  bool override_pause_isolate_flags;
  bool pause_isolate_on_start;
  bool pause_isolate_on_exit;
};

// The DartController may need to request for services to be connected
// to for an isolate that isn't associated with a Mojo application. An
// implementation of this class can be passed to the DartController during
// initialization. ConnectToService requests can come from any thread.
//
// An implementation of this interface is available from the Dart content
// handler, where connections are made via the content handler application.
class DartControllerServiceConnector {
 public:
  // List of services that are supported.
  enum ServiceId {
    kNetworkServiceId,
    kFilesServiceId,
    kNumServiceIds,
  };

  DartControllerServiceConnector() {}
  virtual ~DartControllerServiceConnector() {}

  // Connects to service_id and returns a bare MojoHandle. Calls to this
  // can be made from any thread.
  virtual MojoHandle ConnectToService(ServiceId service_id) = 0;
};

class DartController {
 public:
  // Initializes the Dart VM, and starts up Dart's handle watcher.
  // If strict_compilation is true, the VM runs scripts with assertions and
  // type checking enabled.
  static bool Initialize(DartControllerServiceConnector* service_connector,
                         bool strict_compilation,
                         bool enable_observatory,
                         bool enable_dart_timeline,
                         const char** vm_flags,
                         int vm_flags_count);

  // Setup an isolate to run the program specified in |config|. Invokes
  // the main function and then exits.
  static Dart_Isolate StartupIsolate(const DartControllerConfig& config);

  // Cleanup |isolate|.
  static void ShutdownIsolate(Dart_Isolate isolate);

  // Runs |isolate| to completion. Returns false if isolate exited with error.
  static bool RunToCompletion(Dart_Isolate isolate);

  // Waits for the handle watcher isolate to finish and shuts down the VM.
  static void Shutdown();

  // Does this controller support the 'dart:io' library?
  static bool SupportDartMojoIo();
  // Initialize 'dart:io' for the current isolate.
  static void InitializeDartMojoIo(const std::string& base_uri);
  // Shutdown 'dart:io' for the current isolate.
  static void ShutdownDartMojoIo();

  static Dart_Handle LibraryTagHandler(Dart_LibraryTag tag,
                                       Dart_Handle library,
                                       Dart_Handle url);

 private:
  static void MessageNotifyCallback(Dart_Isolate dest_isolate);

  // Set the control handle in the isolate.
  static Dart_Handle SetHandleWatcherControlHandle();

  // Dart API callback(s).
  static Dart_Isolate IsolateCreateCallback(const char* script_uri,
                                            const char* main,
                                            const char* package_root,
                                            const char* package_config,
                                            Dart_IsolateFlags* flags,
                                            void* callback_data,
                                            char** error);
  static void IsolateShutdownCallback(void* callback_data);

  // Initialize per-isolate flags.
  static void SetIsolateFlags(Dart_IsolateFlags* flags,
                              bool strict_compilation);

  // Dart API callback helper(s).
  static Dart_Isolate CreateIsolateHelper(void* dart_app,
                                          Dart_IsolateFlags* flags,
                                          const DartControllerConfig& config);

  static void InitVmIfNeeded(Dart_EntropySource entropy,
                             bool enable_dart_timeline,
                             const char** vm_flags,
                             int vm_flags_count);

  static void BlockForServiceIsolate();
  static void BlockForServiceIsolateLocked();

  static tonic::DartLibraryProvider* library_provider_;
  static base::Lock lock_;
  static MojoHandle handle_watcher_producer_handle_;
  static bool initialized_;
  static bool strict_compilation_;
  static bool enable_observatory_;
  static bool service_isolate_running_;
  static bool service_isolate_spawned_;
  static DartControllerServiceConnector* service_connector_;
};

}  // namespace dart
}  // namespace mojo

#endif  // MOJO_DART_EMBEDDER_DART_CONTROLLER_H_
