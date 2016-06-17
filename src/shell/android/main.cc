// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/android/main.h"

#include <stdio.h>

#include "base/android/fifo_utils.h"
#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/at_exit.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/synchronization/waitable_event.h"
#include "base/task_runner_util.h"
#include "base/threading/simple_thread.h"
#include "gpu/config/gpu_util.h"
#include "jni/ShellService_jni.h"
#include "mojo/common/binding_set.h"
#include "mojo/message_pump/message_pump_mojo.h"
#include "mojo/services/network/interfaces/network_service.mojom.h"
#include "services/ui/launcher/launcher.mojom.h"
#include "shell/android/android_handler_loader.h"
#include "shell/android/java_application_loader.h"
#include "shell/android/native_viewport_application_loader.h"
#include "shell/android/ui_application_loader_android.h"
#include "shell/android/url_response_disk_cache_delegate_impl.h"
#include "shell/application_manager/application_loader.h"
#include "shell/application_manager/shell_impl.h"
#include "shell/background_application_loader.h"
#include "shell/command_line_util.h"
#include "shell/context.h"
#include "shell/crash/breakpad.h"
#include "shell/crash/crash_upload.h"
#include "shell/init.h"
#include "shell/switches.h"
#include "shell/tracer.h"
#include "ui/gl/gl_surface_egl.h"

using base::LazyInstance;

namespace shell {

namespace {

// Command line argument for the communication fifo.
const char kFifoPath[] = "fifo-path";

// Delay before trying to upload a crash report. This must not slow down
// startup.
const int kDelayBeforeCrashUploadInSeconds = 60;

class MojoShellRunner : public base::DelegateSimpleThread::Delegate {
 public:
  MojoShellRunner(const base::FilePath& mojo_shell_child_path)
      : mojo_shell_child_path_(mojo_shell_child_path) {}
  ~MojoShellRunner() override {}

 private:
  void Run() override;

  const base::FilePath mojo_shell_child_path_;

  DISALLOW_COPY_AND_ASSIGN(MojoShellRunner);
};

// Structure to hold internal data used to setup the Mojo shell.
struct InternalShellData {
  // java_message_loop is the main thread/UI thread message loop.
  scoped_ptr<base::MessageLoop> java_message_loop;

  // tracer, accessible on the main thread
  scoped_ptr<Tracer> tracer;

  // Shell context, accessible on the shell thread
  scoped_ptr<Context> context;

  // Shell runner (thread delegate), to be used by the shell thread.
  scoped_ptr<MojoShellRunner> shell_runner;

  // Thread to run the shell on.
  scoped_ptr<base::DelegateSimpleThread> shell_thread;

  // TaskRunner used to execute tasks on the shell thread.
  scoped_refptr<base::SingleThreadTaskRunner> shell_task_runner;

  // Event signalling when the shell task runner has been initialized.
  scoped_ptr<base::WaitableEvent> shell_runner_ready;

  // Delegate for URLResponseDiskCache. Allows to access bundled application on
  // cold start.
  scoped_ptr<URLResponseDiskCacheDelegateImpl> url_response_disk_cache_delegate;

  // Shell implementation to expose to java.
  scoped_ptr<ShellImpl> shell_impl;

  // Binding set to the shell implementation.
  mojo::BindingSet<mojo::Shell> shell_bindings;

  // Launcher proxy
  launcher::LauncherPtr launcher;
};

LazyInstance<InternalShellData> g_internal_data = LAZY_INSTANCE_INITIALIZER;

void ConfigureAndroidServices(Context* context) {
  context->application_manager()->SetLoaderForURL(
      make_scoped_ptr(new UIApplicationLoader(
          make_scoped_ptr(new NativeViewportApplicationLoader()),
          g_internal_data.Get().java_message_loop.get())),
      GURL("mojo:native_viewport_service"));

  // Android handler is bundled with the Mojo shell, because it uses the
  // MojoShell application as the JNI bridge to bootstrap execution of other
  // Android Mojo apps that need JNI.
  context->application_manager()->SetLoaderForURL(
      make_scoped_ptr(new BackgroundApplicationLoader(
          make_scoped_ptr(new AndroidHandlerLoader()), "android_handler",
          base::MessageLoop::TYPE_DEFAULT)),
      GURL("mojo:android_handler"));

  // Register java applications.
  base::android::ScopedJavaGlobalRef<jobject> java_application_registry(
      JavaApplicationLoader::CreateJavaApplicationRegistry());
  for (const auto& url : JavaApplicationLoader::GetApplicationURLs(
           java_application_registry.obj())) {
    context->application_manager()->SetLoaderForURL(
        make_scoped_ptr(
            new JavaApplicationLoader(java_application_registry, url)),
        GURL(url));
  }

  // By default, the authenticated_network_service is handled by the
  // authentication service.
  context->url_resolver()->AddURLMapping(
      GURL("mojo:authenticated_network_service"), GURL("mojo:authentication"));
}

void QuitShellThread() {
  g_internal_data.Get().shell_thread->Join();
  g_internal_data.Get().shell_thread.reset();
  Java_ShellService_finishActivities(base::android::AttachCurrentThread());
  exit(0);
}

void MojoShellRunner::Run() {
  base::MessageLoop loop(mojo::common::MessagePumpMojo::Create());
  g_internal_data.Get().shell_task_runner = loop.task_runner();
  // Signal that the shell is ready to receive requests.
  g_internal_data.Get().shell_runner_ready->Signal();

  Context* context = g_internal_data.Get().context.get();
  ConfigureAndroidServices(context);
  CHECK(context->InitWithPaths(
      mojo_shell_child_path_,
      g_internal_data.Get().url_response_disk_cache_delegate.get()));

  RunCommandLineApps(context);

  loop.Run();

  g_internal_data.Get().java_message_loop.get()->PostTask(
      FROM_HERE, base::Bind(&QuitShellThread));
}

// Initialize stdout redirection if the command line switch is present.
void InitializeRedirection() {
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(kFifoPath))
    return;

  base::FilePath fifo_path =
      base::CommandLine::ForCurrentProcess()->GetSwitchValuePath(kFifoPath);
  base::FilePath directory = fifo_path.DirName();
  CHECK(base::CreateDirectoryAndGetError(directory, nullptr))
      << "Unable to create directory: " << directory.value();
  unlink(fifo_path.value().c_str());
  CHECK(base::android::CreateFIFO(fifo_path, 0666)) << "Unable to create fifo: "
                                                    << fifo_path.value();
  CHECK(base::android::RedirectStream(stdout, fifo_path, "w"))
      << "Failed to redirect stdout to file: " << fifo_path.value();
  CHECK(dup2(STDOUT_FILENO, STDERR_FILENO) != -1)
      << "Unable to redirect stderr to stdout.";
  // Set stdout to be line buffered to match what one expects when running
  // attached to a terminal.
  if (setvbuf(stdout, NULL, _IONBF, 0) != 0)
    LOG(ERROR) << "Failed to set stdout to be line buffered.";
}

void BindShellImpl(mojo::ScopedMessagePipeHandle shell_handle) {
  Context* context = g_internal_data.Get().context.get();
  if (!g_internal_data.Get().shell_impl.get()) {
    // The application proxy is null, as the shell is not connectable from other
    // applications.
    mojo::ApplicationPtr application;
    // The identity of the shell is the empty URL.
    GURL identity;
    g_internal_data.Get().shell_impl.reset(
        new ShellImpl(application.Pass(), context->application_manager(),
                      Identity(identity), base::Closure()));
  }
  mojo::InterfaceRequest<mojo::Shell> shell;
  shell.Bind(shell_handle.Pass());
  g_internal_data.Get().shell_bindings.AddBinding(
      g_internal_data.Get().shell_impl.get(), shell.Pass());
}

void EmbedApplicationByURL(std::string url) {
  DCHECK(g_internal_data.Get().shell_task_runner->RunsTasksOnCurrentThread());

  if (!g_internal_data.Get().launcher) {
    Context* context = g_internal_data.Get().context.get();
    context->application_manager()->ConnectToService(
        GURL("mojo:launcher"), &g_internal_data.Get().launcher);
  }

  g_internal_data.Get().launcher->Launch(url);
}

void UploadCrashes(const base::FilePath& dumps_path) {
  DCHECK(g_internal_data.Get().shell_task_runner->RunsTasksOnCurrentThread());
  Context* context = g_internal_data.Get().context.get();
  mojo::NetworkServicePtr network_service;
  context->application_manager()->ConnectToService(GURL("mojo:network_service"),
                                                   &network_service);
  breakpad::UploadCrashes(dumps_path, context->task_runners()->blocking_pool(),
                          network_service.Pass());
}

}  // namespace

static void Start(JNIEnv* env,
                  const JavaParamRef<jclass>& clazz,
                  const JavaParamRef<jobject>& application_context,
                  const JavaParamRef<jobject>& j_asset_manager,
                  const JavaParamRef<jstring>& mojo_shell_child_path,
                  const JavaParamRef<jobjectArray>& jparameters,
                  const JavaParamRef<jstring>& j_local_apps_directory,
                  const JavaParamRef<jstring>& j_tmp_dir,
                  const JavaParamRef<jstring>& j_home_dir) {
  // Initially, the shell runner is not ready.
  g_internal_data.Get().shell_runner_ready.reset(
      new base::WaitableEvent(true, false));

  std::string tmp_dir = base::android::ConvertJavaStringToUTF8(env, j_tmp_dir);
  // Setting the TMPDIR and HOME environment variables so that applications can
  // use it.
  // TODO(qsr) We will need our subprocesses to inherit this.
  int return_value = setenv("TMPDIR", tmp_dir.c_str(), 1);
  DCHECK_EQ(return_value, 0);
  return_value = setenv(
      "HOME", base::android::ConvertJavaStringToUTF8(env, j_home_dir).c_str(),
      1);
  DCHECK_EQ(return_value, 0);

  base::android::InitApplicationContext(env, application_context);

  std::vector<std::string> parameters;
  parameters.push_back("mojo_shell");
  base::android::AppendJavaStringArrayToStringVector(env, jparameters,
                                                     &parameters);
  base::CommandLine::Init(0, nullptr);
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  command_line->InitFromArgv(parameters);

  base::FilePath dumps_path = base::FilePath(tmp_dir).Append("breakpad_dumps");
  breakpad::InitCrashReporter(dumps_path);

  Tracer* tracer = new Tracer;
  g_internal_data.Get().tracer.reset(tracer);
  bool trace_startup = command_line->HasSwitch(switches::kTraceStartup);
  if (trace_startup) {
    std::string output_name =
        command_line->GetSwitchValueASCII(switches::kTraceStartupOutputName);
    std::string output_path =
        tmp_dir + "/" +
        (output_name.empty() ? "mojo_shell.trace" : output_name);
    tracer->Start(
        command_line->GetSwitchValueASCII(switches::kTraceStartup),
        command_line->GetSwitchValueASCII(switches::kTraceStartupDuration),
        output_path);
  }

  g_internal_data.Get().shell_runner.reset(new MojoShellRunner(base::FilePath(
      base::android::ConvertJavaStringToUTF8(env, mojo_shell_child_path))));

  InitializeLogging();

  InitializeRedirection();

  // We want ~MessageLoop to happen prior to ~Context. Initializing
  // LazyInstances is akin to stack-allocating objects; their destructors
  // will be invoked first-in-last-out.
  Context* shell_context = new Context(tracer);
  shell_context->SetShellFileRoot(base::FilePath(
      base::android::ConvertJavaStringToUTF8(env, j_local_apps_directory)));
  g_internal_data.Get().context.reset(shell_context);

  g_internal_data.Get().url_response_disk_cache_delegate.reset(
      new URLResponseDiskCacheDelegateImpl(shell_context, j_asset_manager));

  g_internal_data.Get().java_message_loop.reset(new base::MessageLoopForUI);
  base::MessageLoopForUI::current()->Start();
  tracer->DidCreateMessageLoop();

  g_internal_data.Get().shell_thread.reset(new base::DelegateSimpleThread(
      g_internal_data.Get().shell_runner.get(), "ShellThread"));
  g_internal_data.Get().shell_thread->Start();

  // TODO(abarth): At which point should we switch to cross-platform
  // initialization?

  gfx::GLSurface::InitializeOneOff();

  g_internal_data.Get().shell_runner_ready->Wait();

  // Upload crashes after one minute.
  g_internal_data.Get().shell_task_runner->PostDelayedTask(
      FROM_HERE, base::Bind(&UploadCrashes, dumps_path),
      base::TimeDelta::FromSeconds(kDelayBeforeCrashUploadInSeconds));

  gpu::ApplyGpuDriverBugWorkarounds(command_line);
}

static void AddApplicationURL(JNIEnv* env,
                              const JavaParamRef<jclass>& clazz,
                              const JavaParamRef<jstring>& jurl) {
  base::CommandLine::ForCurrentProcess()->AppendArg(
      base::android::ConvertJavaStringToUTF8(env, jurl));
}

static void StartApplicationURL(JNIEnv* env,
                                const JavaParamRef<jclass>& clazz,
                                const JavaParamRef<jstring>& jurl) {
  std::string url = base::android::ConvertJavaStringToUTF8(env, jurl);
  g_internal_data.Get().shell_task_runner->PostTask(
      FROM_HERE, base::Bind(&EmbedApplicationByURL, url));
}

static void BindShell(JNIEnv* env,
                      const JavaParamRef<jclass>& clazz,
                      jint shell_handle) {
  g_internal_data.Get().shell_task_runner->PostTask(
      FROM_HERE,
      base::Bind(&BindShellImpl, base::Passed(mojo::ScopedMessagePipeHandle(
                                     mojo::MessagePipeHandle(shell_handle)))));
}

static void QuitShell(JNIEnv* env, const JavaParamRef<jclass>& clazz) {
  g_internal_data.Get().shell_task_runner->PostTask(
      FROM_HERE, base::MessageLoop::QuitWhenIdleClosure());
}

bool RegisterShellService(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace shell
