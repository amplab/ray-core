// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fcntl.h>

#include "base/files/file_util.h"
#include "base/message_loop/message_loop.h"
#include "base/sha1.h"
#include "base/strings/string_number_conversions.h"
#include "mojo/application/content_handler_factory.h"
#include "mojo/data_pipe_utils/data_pipe_utils.h"
#include "mojo/environment/scoped_chromium_init.h"
#include "mojo/file_utils/file_util.h"
#include "mojo/message_pump/message_pump_mojo.h"
#include "mojo/nacl/nonsfi/file_util.h"
#include "mojo/nacl/nonsfi/nexe_launcher_nonsfi.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl_base.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/public/cpp/bindings/array.h"
#include "mojo/public/cpp/bindings/synchronous_interface_ptr.h"
#include "mojo/services/files/interfaces/directory.mojom-sync.h"
#include "mojo/services/files/interfaces/files.mojom.h"
#include "services/nacl/nonsfi/pnacl_compile.mojom-sync.h"
#include "services/nacl/nonsfi/pnacl_link.mojom-sync.h"

namespace nacl {
namespace content_handler {

class PexeContentHandler : public mojo::ApplicationImplBase,
                           public mojo::ContentHandlerFactory::Delegate {
 public:
  PexeContentHandler() {}

 private:
  // Overridden from ApplicationImplBase:
  void OnInitialize() override {
    mojo::ConnectToService(shell(), "mojo:pnacl_compile",
                           GetSynchronousProxy(&compiler_init_));
    mojo::ConnectToService(shell(), "mojo:pnacl_link",
                           GetSynchronousProxy(&linker_init_));
    mojo::ConnectToService(shell(), "mojo:files", GetProxy(&files_));
    mojo::files::Error error = mojo::files::Error::INTERNAL;
    files_->OpenFileSystem("app_persistent_cache",
                           GetSynchronousProxy(&nexe_cache_directory_),
                           [&error](mojo::files::Error e) { error = e; });
    CHECK(files_.WaitForIncomingResponse());
    CHECK_EQ(mojo::files::Error::OK, error);
  }

  // Overridden from ApplicationImplBase:
  bool OnAcceptConnection(
      mojo::ServiceProviderImpl* service_provider_impl) override {
    service_provider_impl->AddService<mojo::ContentHandler>(
        mojo::ContentHandlerFactory::GetInterfaceRequestHandler(this));
    return true;
  }

  int AccessFileFromCache(std::string& digest) {
    mojo::files::Error error = mojo::files::Error::INTERNAL;
    mojo::files::FilePtr nexe_cache_file;
    CHECK(nexe_cache_directory_->OpenFile(digest, GetProxy(&nexe_cache_file),
                                          mojo::files::kOpenFlagRead, &error));
    if (mojo::files::Error::OK == error)
      // Copy the mojo cached file into an open temporary file.
      return ::nacl::MojoFileToTempFileDescriptor(nexe_cache_file.Pass());
    else
      // If error != OK, The failure may have been for a variety of reasons --
      // assume that the file does not exist.
      return -1;
  }

  void StoreFileInCache(int nexe_fd, std::string& digest) {
    // First, open a "temporary" file.
    mojo::files::Error error = mojo::files::Error::INTERNAL;
    std::string temp_file_name;
    auto nexe_cache_file =
        mojo::files::FilePtr::Create(file_utils::CreateTemporaryFileInDir(
            &nexe_cache_directory_, &temp_file_name));
    CHECK(nexe_cache_file);

    // Copy the contents of nexe_fd into the temporary Mojo file.
    FileDescriptorToMojoFile(nexe_fd, nexe_cache_file.Pass());

    // The file is named after the hash of the requesting pexe.
    // This makes it usable by future requests for the same pexe under different
    // names. It also atomically moves the entire temp file.
    CHECK(nexe_cache_directory_->Rename(temp_file_name, digest, &error));
    CHECK_EQ(mojo::files::Error::OK, error);
  }

  int DoPexeTranslation(base::FilePath& pexe_file_path) {
    // Compile the pexe into an object file
    mojo::SynchronousInterfacePtr<mojo::nacl::PexeCompiler> compiler;
    CHECK(compiler_init_->PexeCompilerStart(GetSynchronousProxy(&compiler)));

    // Communicate with the compiler using a mojom interface.
    mojo::Array<mojo::String> object_files;
    CHECK(compiler->PexeCompile(pexe_file_path.value(), &object_files));

    // Link the object file into a nexe
    mojo::SynchronousInterfacePtr<mojo::nacl::PexeLinker> linker;
    CHECK(linker_init_->PexeLinkerStart(GetSynchronousProxy(&linker)));

    // Communicate with the linker using a mojom interface.
    mojo::String nexe_file;
    CHECK(linker->PexeLink(std::move(object_files), &nexe_file));

    // Open the nexe file and launch it (with our mojo handle)
    int nexe_fd = open(nexe_file.get().c_str(), O_RDONLY);
    CHECK(!unlink(nexe_file.get().c_str()))
        << "Could not unlink temporary nexe file";
    CHECK_GE(nexe_fd, 0) << "Could not open nexe object file";
    return nexe_fd;
  }

  // Overridden from ContentHandlerFactory::Delegate:
  void RunApplication(
      mojo::InterfaceRequest<mojo::Application> application_request,
      mojo::URLResponsePtr response) override {
    // Needed to use Mojo interfaces on this thread.
    base::MessageLoop loop(mojo::common::MessagePumpMojo::Create());
    // Create temporary file for pexe
    base::FilePath pexe_file_path;
    FILE* pexe_fp = CreateAndOpenTemporaryFile(&pexe_file_path);
    CHECK(pexe_fp) << "Could not create temporary file for pexe";
    // Acquire the pexe.
    CHECK(mojo::common::BlockingCopyToFile(response->body.Pass(), pexe_fp))
        << "Could not copy pexe to file";
    CHECK_EQ(fclose(pexe_fp), 0) << "Could not close pexe file";

    // Try to access the translated pexe from the cache based on the
    // SHA1 hash of the pexe itself.
    unsigned char raw_hash[base::kSHA1Length];
    CHECK(base::SHA1HashFile(pexe_file_path.value().c_str(), raw_hash));

    std::string digest = base::HexEncode(raw_hash, sizeof(raw_hash));
    int nexe_fd = AccessFileFromCache(digest);
    if (nexe_fd == -1) {
      nexe_fd = DoPexeTranslation(pexe_file_path);
      // Store the nexe in the cache for the next translation
      StoreFileInCache(nexe_fd, digest);
    }

    // Pass the handle connecting us with mojo_shell to the nexe.
    MojoHandle handle = application_request.PassMessagePipe().release().value();
    ::nacl::MojoLaunchNexeNonsfi(nexe_fd, handle,
                                 false /* enable_translation_irt */);
  }

 private:
  mojo::SynchronousInterfacePtr<mojo::files::Directory> nexe_cache_directory_;
  mojo::files::FilesPtr files_;
  mojo::SynchronousInterfacePtr<mojo::nacl::PexeCompilerInit> compiler_init_;
  mojo::SynchronousInterfacePtr<mojo::nacl::PexeLinkerInit> linker_init_;

  DISALLOW_COPY_AND_ASSIGN(PexeContentHandler);
};

}  // namespace content_handler
}  // namespace nacl

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ScopedChromiumInit init;
  nacl::content_handler::PexeContentHandler pexe_content_handler;
  return mojo::RunApplication(application_request, &pexe_content_handler);
}
