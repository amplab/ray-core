// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <dlfcn.h>
#include <python2.7/Python.h>

#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/i18n/icu_util.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "mojo/application/content_handler_factory.h"
#include "mojo/data_pipe_utils/data_pipe_utils.h"
#include "mojo/environment/scoped_chromium_init.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/python/src/common.h"
#include "third_party/zlib/google/zip_reader.h"
#include "url/gurl.h"

char kMojoSystem[] = "mojo_system";
char kMojoSystemImpl[] = "mojo_system_impl";
char kMojoMain[] = "MojoMain";

extern "C" {
  void initmojo_system();
  void initmojo_system_impl();
}

namespace services {
namespace python {
namespace content_handler {

using mojo::Application;
using mojo::ApplicationImplBase;
using mojo::ContentHandlerFactory;
using mojo::InterfaceRequest;
using mojo::ScopedDataPipeConsumerHandle;
using mojo::URLResponsePtr;
using mojo::python::ScopedPyRef;

class PythonContentHandler : public ContentHandlerFactory::Delegate {
 public:
  PythonContentHandler(bool debug) : debug_(debug) {}

 private:
  // Extracts the target application into a temporary directory. This directory
  // is deleted at the end of the life of the PythonContentHandler object.
  std::unique_ptr<base::ScopedTempDir> ExtractApplication(
      URLResponsePtr response) {
    std::unique_ptr<base::ScopedTempDir> temp_dir(new base::ScopedTempDir);
    CHECK(temp_dir->CreateUniqueTempDir());

    zip::ZipReader reader;
    const std::string input_data = CopyToString(response->body.Pass());
    CHECK(reader.OpenFromString(input_data));
    base::FilePath temp_dir_path = temp_dir->path();
    while (reader.HasMore()) {
      CHECK(reader.OpenCurrentEntryInZip());
      CHECK(reader.ExtractCurrentEntryIntoDirectory(temp_dir_path));
      CHECK(reader.AdvanceToNextEntry());
    }
    return temp_dir;
  }

  ScopedPyRef RunString(std::string command, PyObject* globals, int mode) {
    ScopedPyRef result(PyRun_String(command.c_str(), mode, globals, nullptr));

    if (!result) {
      LOG(ERROR) << "Error while running command: '" << command << "'";
      PyErr_Print();
    }
    return result;
  }

  bool Exec(std::string command, PyObject* globals) {
    return RunString(command, globals, Py_file_input);
  }

  ScopedPyRef Eval(std::string command, PyObject* globals) {
    return RunString(command, globals, Py_eval_input);
  }

  // Sets up the Python interpreter and loads mojo system modules. This method
  // returns the global dictionary for the python environment that should be
  // used for subsequent calls. Takes as input the path of the unpacked
  // application files.
  PyObject* SetupPythonEnvironment(const std::string& application_path) {
    // TODO(etiennej): Build python ourselves so we don't have to rely on
    // dynamically loading a system library.
    dlopen("libpython2.7.so", RTLD_LAZY | RTLD_GLOBAL);

    PyImport_AppendInittab(kMojoSystemImpl, &initmojo_system_impl);
    PyImport_AppendInittab(kMojoSystem, &initmojo_system);

    Py_Initialize();

    PyObject *m, *d;
    m = PyImport_AddModule("__main__");
    if (!m)
      return nullptr;
    d = PyModule_GetDict(m);

    // Enable debug information if requested.
    if (debug_) {
      std::string enable_logging =
          "import logging;"
          "logging.basicConfig();"
          "logging.getLogger().setLevel(logging.DEBUG)";
      if (!Exec(enable_logging, d))
        return nullptr;
    }

    // Inject the application path into the python search path so that imports
    // from the application work as expected.
    std::string search_path_py_command =
        "import sys; sys.path.append('" + application_path + "');";
    if (!Exec(search_path_py_command, d))
      return nullptr;

    return d;
  }

  // Overridden from ContentHandlerFactory::ManagedDelegate:
  void RunApplication(InterfaceRequest<Application> application_request,
                      URLResponsePtr response) override {
    std::unique_ptr<base::ScopedTempDir> temp_dir =
        ExtractApplication(response.Pass());
    base::FilePath directory_path = temp_dir->path();

    PyObject* d = SetupPythonEnvironment(directory_path.value());
    DCHECK(d);

    base::FilePath entry_path = directory_path.Append("__mojo__.py");

    FILE* entry_file = base::OpenFile(entry_path, "r");
    DCHECK(entry_file);

    // Ensure that all created objects are destroyed before calling Py_Finalize.
    {
      // Load the __mojo__.py file into the interpreter. MojoMain hasn't run
      // yet.
      ScopedPyRef result(PyRun_File(entry_file, entry_path.value().c_str(),
                                    Py_file_input, d, d));
      if (!result) {
        LOG(ERROR) << "Error while loading script";
        PyErr_Print();
        return;
      }

      // Find MojoMain.
      ScopedPyRef py_function(PyMapping_GetItemString(d, kMojoMain));

      if (!py_function) {
        LOG(ERROR) << "Locals size: " << PyMapping_Size(d);
        LOG(ERROR) << "MojoMain not found";
        PyErr_Print();
        return;
      }

      if (PyCallable_Check(py_function)) {
        MojoHandle application_request_handle =
            application_request.PassMessagePipe().release().value();
        std::string handle_command = base::StringPrintf(
            "mojo_system.Handle(%u)", application_request_handle);
        ScopedPyRef py_input(Eval(handle_command, d));
        if (!py_input) {
          MojoClose(application_request_handle);
          return;
        }
        ScopedPyRef py_arguments(PyTuple_New(1));
        // py_input reference is stolen by py_arguments
        PyTuple_SetItem(py_arguments, 0, py_input.Release());
        // Run MojoMain with application_request_handle as the first and only
        // argument.
        ScopedPyRef py_output(PyObject_CallObject(py_function, py_arguments));

        if (!py_output) {
          LOG(ERROR) << "Error while executing MojoMain";
          PyErr_Print();
          return;
        }
      } else {
        LOG(ERROR) << "MojoMain is not callable; it should be a function";
      }
    }
    Py_Finalize();
  }

  std::string CopyToString(ScopedDataPipeConsumerHandle body) {
    std::string body_str;
    bool result = mojo::common::BlockingCopyToString(body.Pass(), &body_str);
    DCHECK(result);
    return body_str;
  }

  bool debug_;

  DISALLOW_COPY_AND_ASSIGN(PythonContentHandler);
};

class PythonContentHandlerApp : public ApplicationImplBase {
 public:
  PythonContentHandlerApp()
      : content_handler_(false), debug_content_handler_(true) {}

 private:
  // Overridden from ApplicationImplBase:
  bool OnAcceptConnection(
      mojo::ServiceProviderImpl* service_provider_impl) override {
    if (IsDebug(service_provider_impl->connection_context().connection_url)) {
      service_provider_impl->AddService<mojo::ContentHandler>(
          ContentHandlerFactory::GetInterfaceRequestHandler(
              &debug_content_handler_));
    } else {
      service_provider_impl->AddService<mojo::ContentHandler>(
          ContentHandlerFactory::GetInterfaceRequestHandler(&content_handler_));
    }
    return true;
  }

  bool IsDebug(const std::string& requestedUrl) {
    GURL url(requestedUrl);
    if (url.has_query()) {
      std::vector<std::string> query_parameters = base::SplitString(
          url.query(), "&", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
      return std::find(query_parameters.begin(), query_parameters.end(),
                       "debug=true") != query_parameters.end();
    }
    return false;
  }

  PythonContentHandler content_handler_;
  PythonContentHandler debug_content_handler_;

  DISALLOW_COPY_AND_ASSIGN(PythonContentHandlerApp);
};

}  // namespace content_handler
}  // namespace python
}  // namespace services

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ScopedChromiumInit init;
  services::python::content_handler::PythonContentHandlerApp
      python_content_handler_app;
  return mojo::RunApplication(application_request, &python_content_handler_app);
}
