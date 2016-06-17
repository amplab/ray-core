// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file was forked from components/crash/app/breakpad_linux.cc and
// components/crash/app/breakpad_linux_impl.h in chromium.

// For linux_syscall_support.h. This makes it safe to call embedded system
// calls when in seccomp mode.

#include "shell/crash/breakpad.h"

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <algorithm>
#include <string>

#include "base/command_line.h"
#include "base/debug/crash_logging.h"
#include "base/debug/dump_without_crashing.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/linux_util.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/posix/eintr_wrapper.h"
#include "base/process/memory.h"
#include "base/strings/string_util.h"
#include "build/build_config.h"
#include "third_party/breakpad/src/client/linux/crash_generation/crash_generation_client.h"
#include "third_party/breakpad/src/client/linux/handler/exception_handler.h"
#include "third_party/breakpad/src/client/linux/minidump_writer/directory_reader.h"
#include "third_party/breakpad/src/common/linux/linux_libc_support.h"
#include "third_party/breakpad/src/common/memory.h"
#include "third_party/breakpad/src/common/simple_string_dictionary.h"

#if defined(OS_ANDROID)
#include <android/log.h>
#include <sys/stat.h>

#include "base/android/build_info.h"
#include "base/android/path_utils.h"
#endif
#include "third_party/lss/linux_syscall_support.h"

#if defined(OS_ANDROID)
#define STAT_STRUCT struct stat
#define FSTAT_FUNC fstat
#else
#define STAT_STRUCT struct kernel_stat
#define FSTAT_FUNC sys_fstat
#endif

// Some versions of gcc are prone to warn about unused return values. In cases
// where we either a) know the call cannot fail, or b) there is nothing we
// can do when a call fails, we mark the return code as ignored. This avoids
// spurious compiler warnings.
#define IGNORE_RET(x) \
  do {                \
    if (x)            \
      ;               \
  } while (0)

using google_breakpad::ExceptionHandler;
using google_breakpad::MinidumpDescriptor;

namespace breakpad {

namespace {

using CrashKeyStorage = google_breakpad::NonAllocatingMap<256, 256, 64>;

// BreakpadInfo describes a crash report.
// The minidump information can either be contained in a file descriptor (fd) or
// in a file (whose path is in filename).
struct BreakpadInfo {
  int fd;                        // File descriptor to the Breakpad dump data.
  const char* filename;          // Path to the Breakpad dump data.
  const char* process_type;      // Process type, e.g. "renderer".
  unsigned process_type_length;  // Length of |process_type|.
  const char* distro;            // Linux distro string.
  unsigned distro_length;        // Length of |distro|.
  uint64_t process_start_time;   // Uptime of the crashing process.
  size_t oom_size;               // Amount of memory requested if OOM.
  uint64_t pid;                  // PID where applicable.
  CrashKeyStorage* crash_keys;
};

// Define a preferred limit on minidump sizes, because Crash Server currently
// throws away any larger than 1.2MB (1.2 * 1024 * 1024).  A value of -1 means
// no limit.
static const off_t kMaxMinidumpFileSize = 1258291;
bool g_is_crash_reporter_enabled = false;
uint64_t g_process_start_time = 0;
pid_t g_pid = 0;
ExceptionHandler* g_breakpad = nullptr;

CrashKeyStorage* g_crash_keys = nullptr;

#if defined(OS_ANDROID)
const char kProductName[] = "Mojo_Android";
#else
const char kProductName[] = "Mojo";
#endif
const char kVersion[] = "1.0.0";

// Writes the value |v| as 16 hex characters to the memory pointed at by
// |output|.
void write_uint64_hex(char* output, uint64_t v) {
  static const char hextable[] = "0123456789abcdef";

  for (int i = 15; i >= 0; --i) {
    output[i] = hextable[v & 15];
    v >>= 4;
  }
}

// The following helper functions are for calculating uptime.

// Converts a struct timeval to milliseconds.
uint64_t timeval_to_ms(struct timeval* tv) {
  uint64_t ret = tv->tv_sec;  // Avoid overflow by explicitly using a uint64_t.
  ret *= 1000;
  ret += tv->tv_usec / 1000;
  return ret;
}

// Converts a struct timeval to milliseconds.
uint64_t kernel_timeval_to_ms(struct kernel_timeval* tv) {
  uint64_t ret = tv->tv_sec;  // Avoid overflow by explicitly using a uint64_t.
  ret *= 1000;
  ret += tv->tv_usec / 1000;
  return ret;
}

// String buffer size to use to convert a uint64_t to string.
const size_t kUint64StringSize = 21;

void SetProcessStartTime() {
  // Set the base process start time value.
  struct timeval tv;
  if (!gettimeofday(&tv, nullptr))
    g_process_start_time = timeval_to_ms(&tv);
  else
    g_process_start_time = 0;
}

// uint64_t version of my_int_len() from
// breakpad/src/common/linux/linux_libc_support.h. Return the length of the
// given, non-negative integer when expressed in base 10.
unsigned my_uint64_len(uint64_t i) {
  if (!i)
    return 1;

  unsigned len = 0;
  while (i) {
    len++;
    i /= 10;
  }

  return len;
}

// uint64_t version of my_uitos() from
// breakpad/src/common/linux/linux_libc_support.h. Convert a non-negative
// integer to a string (not null-terminated).
void my_uint64tos(char* output, uint64_t i, unsigned i_len) {
  for (unsigned index = i_len; index; --index, i /= 10)
    output[index - 1] = '0' + (i % 10);
}

size_t LengthWithoutTrailingSpaces(const char* str, size_t len) {
  while (len > 0 && str[len - 1] == ' ') {
    len--;
  }
  return len;
}

// MIME substrings.
const char g_rn[] = "\r\n";
const char g_form_data_msg[] = "Content-Disposition: form-data; name=\"";
const char g_quote_msg[] = "\"";
const char g_dashdash_msg[] = "--";
const char g_dump_msg[] = "upload_file_minidump\"; filename=\"dump\"";
const char g_content_type_msg[] = "Content-Type: application/octet-stream";

// MimeWriter manages an iovec for writing MIMEs to a file.
class MimeWriter {
 public:
  static const int kIovCapacity = 30;
  static const size_t kMaxCrashChunkSize = 64;

  MimeWriter(int fd, const char* const mime_boundary);
  ~MimeWriter();

  // Append boundary.
  virtual void AddBoundary();

  // Append end of file boundary.
  virtual void AddEnd();

  // Append key/value pair with specified sizes.
  virtual void AddPairData(const char* msg_type,
                           size_t msg_type_size,
                           const char* msg_data,
                           size_t msg_data_size);

  // Append key/value pair.
  void AddPairString(const char* msg_type, const char* msg_data) {
    AddPairData(msg_type, my_strlen(msg_type), msg_data, my_strlen(msg_data));
  }

  // Append key/value pair, splitting value into chunks no larger than
  // |chunk_size|. |chunk_size| cannot be greater than |kMaxCrashChunkSize|.
  // The msg_type string will have a counter suffix to distinguish each chunk.
  virtual void AddPairDataInChunks(const char* msg_type,
                                   size_t msg_type_size,
                                   const char* msg_data,
                                   size_t msg_data_size,
                                   size_t chunk_size,
                                   bool strip_trailing_spaces);

  // Add binary file contents to be uploaded with the specified filename.
  virtual void AddFileContents(const char* filename_msg,
                               uint8_t* file_data,
                               size_t file_size);

  // Flush any pending iovecs to the output file.
  void Flush() {
    IGNORE_RET(sys_writev(fd_, iov_, iov_index_));
    iov_index_ = 0;
  }

 protected:
  void AddItem(const void* base, size_t size);
  // Minor performance trade-off for easier-to-maintain code.
  void AddString(const char* str) { AddItem(str, my_strlen(str)); }
  void AddItemWithoutTrailingSpaces(const void* base, size_t size);

  struct kernel_iovec iov_[kIovCapacity];
  int iov_index_;

  // Output file descriptor.
  int fd_;

  const char* const mime_boundary_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MimeWriter);
};

MimeWriter::MimeWriter(int fd, const char* const mime_boundary)
    : iov_index_(0), fd_(fd), mime_boundary_(mime_boundary) {
}

MimeWriter::~MimeWriter() {
}

void MimeWriter::AddBoundary() {
  AddString(mime_boundary_);
  AddString(g_rn);
}

void MimeWriter::AddEnd() {
  AddString(mime_boundary_);
  AddString(g_dashdash_msg);
  AddString(g_rn);
}

void MimeWriter::AddPairData(const char* msg_type,
                             size_t msg_type_size,
                             const char* msg_data,
                             size_t msg_data_size) {
  AddString(g_form_data_msg);
  AddItem(msg_type, msg_type_size);
  AddString(g_quote_msg);
  AddString(g_rn);
  AddString(g_rn);
  AddItem(msg_data, msg_data_size);
  AddString(g_rn);
}

void MimeWriter::AddPairDataInChunks(const char* msg_type,
                                     size_t msg_type_size,
                                     const char* msg_data,
                                     size_t msg_data_size,
                                     size_t chunk_size,
                                     bool strip_trailing_spaces) {
  if (chunk_size > kMaxCrashChunkSize)
    return;

  unsigned i = 0;
  size_t done = 0, msg_length = msg_data_size;

  while (msg_length) {
    char num[kUint64StringSize];
    const unsigned num_len = my_uint_len(++i);
    my_uitos(num, i, num_len);

    size_t chunk_len = std::min(chunk_size, msg_length);

    AddString(g_form_data_msg);
    AddItem(msg_type, msg_type_size);
    AddItem(num, num_len);
    AddString(g_quote_msg);
    AddString(g_rn);
    AddString(g_rn);
    if (strip_trailing_spaces) {
      AddItemWithoutTrailingSpaces(msg_data + done, chunk_len);
    } else {
      AddItem(msg_data + done, chunk_len);
    }
    AddString(g_rn);
    AddBoundary();
    Flush();

    done += chunk_len;
    msg_length -= chunk_len;
  }
}

void MimeWriter::AddFileContents(const char* filename_msg,
                                 uint8_t* file_data,
                                 size_t file_size) {
  AddString(g_form_data_msg);
  AddString(filename_msg);
  AddString(g_rn);
  AddString(g_content_type_msg);
  AddString(g_rn);
  AddString(g_rn);
  AddItem(file_data, file_size);
  AddString(g_rn);
}

void MimeWriter::AddItem(const void* base, size_t size) {
  // Check if the iovec is full and needs to be flushed to output file.
  if (iov_index_ == kIovCapacity) {
    Flush();
  }
  iov_[iov_index_].iov_base = const_cast<void*>(base);
  iov_[iov_index_].iov_len = size;
  ++iov_index_;
}

void MimeWriter::AddItemWithoutTrailingSpaces(const void* base, size_t size) {
  AddItem(base,
          LengthWithoutTrailingSpaces(static_cast<const char*>(base), size));
}

void DumpProcess() {
  if (g_breakpad)
    g_breakpad->WriteMinidump();
}

#if defined(OS_ANDROID)
const char kGoogleBreakpad[] = "google-breakpad";
#endif

size_t WriteLog(const char* buf, size_t nbytes) {
#if defined(OS_ANDROID)
  return __android_log_write(ANDROID_LOG_WARN, kGoogleBreakpad, buf);
#else
  return sys_write(2, buf, nbytes);
#endif
}

#if defined(OS_ANDROID)
void AndroidLogWriteHorizontalRule() {
  __android_log_write(ANDROID_LOG_WARN, kGoogleBreakpad,
                      "### ### ### ### ### ### ### ### ### ### ### ### ###");
}

// Android's native crash handler outputs a diagnostic tombstone to the device
// log. By returning false from the HandlerCallbacks, breakpad will reinstall
// the previous (i.e. native) signal handlers before returning from its own
// handler. A Mojo shell build fingerprint is written to the log, so that the
// specific build of the shell and the location of the archived shell symbols
// can be determined directly from it.
bool FinalizeCrashDoneAndroid() {
  base::android::BuildInfo* android_build_info =
      base::android::BuildInfo::GetInstance();

  AndroidLogWriteHorizontalRule();
  __android_log_write(ANDROID_LOG_WARN, kGoogleBreakpad,
                      "Mojo shell build fingerprint:");
  __android_log_write(ANDROID_LOG_WARN, kGoogleBreakpad,
                      android_build_info->package_version_name());
  __android_log_write(ANDROID_LOG_WARN, kGoogleBreakpad,
                      android_build_info->package_version_code());
  AndroidLogWriteHorizontalRule();

  return false;
}
#endif

void LoadDataFromFD(google_breakpad::PageAllocator& allocator,
                    int fd,
                    bool close_fd,
                    uint8_t** file_data,
                    size_t* size) {
  STAT_STRUCT st;
  if (FSTAT_FUNC(fd, &st) != 0) {
    static const char msg[] = "Cannot upload crash dump: stat failed\n";
    WriteLog(msg, sizeof(msg) - 1);
    if (close_fd)
      IGNORE_RET(sys_close(fd));
    return;
  }

  *file_data = reinterpret_cast<uint8_t*>(allocator.Alloc(st.st_size));
  if (!(*file_data)) {
    static const char msg[] = "Cannot upload crash dump: cannot alloc\n";
    WriteLog(msg, sizeof(msg) - 1);
    if (close_fd)
      IGNORE_RET(sys_close(fd));
    return;
  }
  my_memset(*file_data, 0xf, st.st_size);

  *size = st.st_size;
  int byte_read = sys_read(fd, *file_data, *size);
  if (byte_read == -1) {
    static const char msg[] = "Cannot upload crash dump: read failed\n";
    WriteLog(msg, sizeof(msg) - 1);
    if (close_fd)
      IGNORE_RET(sys_close(fd));
    return;
  }

  if (close_fd)
    IGNORE_RET(sys_close(fd));
}

void LoadDataFromFile(google_breakpad::PageAllocator& allocator,
                      const char* filename,
                      int* fd,
                      uint8_t** file_data,
                      size_t* size) {
  // WARNING: this code runs in a compromised context. It may not call into
  // libc nor allocate memory normally.
  *fd = sys_open(filename, O_RDONLY, 0);
  *size = 0;

  if (*fd < 0) {
    static const char msg[] = "Cannot upload crash dump: failed to open\n";
    WriteLog(msg, sizeof(msg) - 1);
    return;
  }

  LoadDataFromFD(allocator, *fd, true, file_data, size);
}

void HandleCrashDump(const BreakpadInfo& info) {
  int dumpfd;
  bool keep_fd = false;
  size_t dump_size;
  uint8_t* dump_data;
  google_breakpad::PageAllocator allocator;

  if (info.fd != -1) {
    // Dump is provided with an open FD.
    keep_fd = true;
    dumpfd = info.fd;

    // The FD is pointing to the end of the file.
    // Rewind, we'll read the data next.
    if (lseek(dumpfd, 0, SEEK_SET) == -1) {
      static const char msg[] =
          "Cannot upload crash dump: failed to "
          "reposition minidump FD\n";
      WriteLog(msg, sizeof(msg) - 1);
      IGNORE_RET(sys_close(dumpfd));
      return;
    }
    LoadDataFromFD(allocator, info.fd, false, &dump_data, &dump_size);
  } else {
    // Dump is provided with a path.
    keep_fd = false;
    LoadDataFromFile(allocator, info.filename, &dumpfd, &dump_data, &dump_size);
  }

  // We need to build a MIME block for uploading to the server. Since we are
  // going to fork and run wget, it needs to be written to a temp file.
  const int ufd = sys_open("/dev/urandom", O_RDONLY, 0);
  if (ufd < 0) {
    static const char msg[] =
        "Cannot upload crash dump because /dev/urandom"
        " is missing\n";
    WriteLog(msg, sizeof(msg) - 1);
    return;
  }

  int temp_file_fd = -1;
  if (keep_fd) {
    temp_file_fd = dumpfd;
    // Rewind the destination, we are going to overwrite it.
    if (lseek(dumpfd, 0, SEEK_SET) == -1) {
      static const char msg[] =
          "Cannot upload crash dump: failed to "
          "reposition minidump FD (2)\n";
      WriteLog(msg, sizeof(msg) - 1);
      IGNORE_RET(sys_close(dumpfd));
      return;
    }
  } else {
    temp_file_fd = sys_open(info.filename, O_WRONLY, 0600);
    if (temp_file_fd < 0) {
      static const char msg[] = "Failed to save crash dump: failed to open\n";
      WriteLog(msg, sizeof(msg) - 1);
      IGNORE_RET(sys_close(ufd));
      return;
    }
  }

  // The MIME boundary is 28 hyphens, followed by a 64-bit nonce and a NUL.
  char mime_boundary[28 + 16 + 1];
  my_memset(mime_boundary, '-', 28);
  uint64_t boundary_rand;
  sys_read(ufd, &boundary_rand, sizeof(boundary_rand));
  write_uint64_hex(mime_boundary + 28, boundary_rand);
  mime_boundary[28 + 16] = 0;
  IGNORE_RET(sys_close(ufd));

  // The MIME block looks like this:
  //   BOUNDARY \r\n
  //   Content-Disposition: form-data; name="prod" \r\n \r\n
  //   MojoShell \r\n
  //   BOUNDARY \r\n
  //   Content-Disposition: form-data; name="ver" \r\n \r\n
  //   1.2.3.4 \r\n
  //   BOUNDARY \r\n
  //
  //   zero or one:
  //   Content-Disposition: form-data; name="ptime" \r\n \r\n
  //   abcdef \r\n
  //   BOUNDARY \r\n
  //
  //   zero or one:
  //   Content-Disposition: form-data; name="ptype" \r\n \r\n
  //   abcdef \r\n
  //   BOUNDARY \r\n
  //
  //   zero or one:
  //   Content-Disposition: form-data; name="lsb-release" \r\n \r\n
  //   abcdef \r\n
  //   BOUNDARY \r\n
  //
  //   zero or one:
  //   Content-Disposition: form-data; name="oom-size" \r\n \r\n
  //   1234567890 \r\n
  //   BOUNDARY \r\n
  //
  //   zero or more (up to CrashKeyStorage::num_entries = 64):
  //   Content-Disposition: form-data; name=crash-key-name \r\n
  //   crash-key-value \r\n
  //   BOUNDARY \r\n
  //
  //   Content-Disposition: form-data; name="dump"; filename="dump" \r\n
  //   Content-Type: application/octet-stream \r\n \r\n
  //   <dump contents>
  //   \r\n BOUNDARY -- \r\n

  MimeWriter writer(temp_file_fd, mime_boundary);
  {
    writer.AddBoundary();
    writer.AddPairString("prod", kProductName);
    writer.AddBoundary();
    writer.AddPairString("ver", kVersion);
    writer.AddBoundary();
    if (info.pid > 0) {
      char pid_value_buf[kUint64StringSize];
      uint64_t pid_value_len = my_uint64_len(info.pid);
      my_uint64tos(pid_value_buf, info.pid, pid_value_len);
      static const char pid_key_name[] = "pid";
      writer.AddPairData(pid_key_name, sizeof(pid_key_name) - 1, pid_value_buf,
                         pid_value_len);
      writer.AddBoundary();
    }
#if defined(OS_ANDROID)
    // Addtional MIME blocks are added for logging on Android devices.
    static const char android_build_id[] = "android_build_id";
    static const char android_build_fp[] = "android_build_fp";
    static const char device[] = "device";
    static const char model[] = "model";
    static const char brand[] = "brand";
    static const char exception_info[] = "exception_info";

    base::android::BuildInfo* android_build_info =
        base::android::BuildInfo::GetInstance();
    writer.AddPairString(android_build_id,
                         android_build_info->android_build_id());
    writer.AddBoundary();
    writer.AddPairString(android_build_fp,
                         android_build_info->android_build_fp());
    writer.AddBoundary();
    writer.AddPairString(device, android_build_info->device());
    writer.AddBoundary();
    writer.AddPairString(model, android_build_info->model());
    writer.AddBoundary();
    writer.AddPairString(brand, android_build_info->brand());
    writer.AddBoundary();
    if (android_build_info->java_exception_info() != nullptr) {
      writer.AddPairString(exception_info,
                           android_build_info->java_exception_info());
      writer.AddBoundary();
    }
#endif
    writer.Flush();
  }

  if (info.process_start_time > 0) {
    struct kernel_timeval tv;
    if (!sys_gettimeofday(&tv, nullptr)) {
      uint64_t time = kernel_timeval_to_ms(&tv);
      if (time > info.process_start_time) {
        time -= info.process_start_time;
        char time_str[kUint64StringSize];
        const unsigned time_len = my_uint64_len(time);
        my_uint64tos(time_str, time, time_len);

        static const char process_time_msg[] = "ptime";
        writer.AddPairData(process_time_msg, sizeof(process_time_msg) - 1,
                           time_str, time_len);
        writer.AddBoundary();
        writer.Flush();
      }
    }
  }

  if (info.process_type_length) {
    writer.AddPairString("ptype", info.process_type);
    writer.AddBoundary();
    writer.Flush();
  }

  if (info.distro_length) {
    static const char distro_msg[] = "lsb-release";
    writer.AddPairString(distro_msg, info.distro);
    writer.AddBoundary();
    writer.Flush();
  }

  if (info.oom_size) {
    char oom_size_str[kUint64StringSize];
    const unsigned oom_size_len = my_uint64_len(info.oom_size);
    my_uint64tos(oom_size_str, info.oom_size, oom_size_len);
    static const char oom_size_msg[] = "oom-size";
    writer.AddPairData(oom_size_msg, sizeof(oom_size_msg) - 1, oom_size_str,
                       oom_size_len);
    writer.AddBoundary();
    writer.Flush();
  }

  if (info.crash_keys) {
    CrashKeyStorage::Iterator crash_key_iterator(*info.crash_keys);
    const CrashKeyStorage::Entry* entry;
    while ((entry = crash_key_iterator.Next())) {
      writer.AddPairString(entry->key, entry->value);
      writer.AddBoundary();
      writer.Flush();
    }
  }

  writer.AddFileContents(g_dump_msg, dump_data, dump_size);
  writer.AddEnd();
  writer.Flush();

  IGNORE_RET(sys_close(temp_file_fd));
}

bool CrashDone(const MinidumpDescriptor& minidump, const bool succeeded) {
  // WARNING: this code runs in a compromised context. It may not call into
  // libc nor allocate memory normally.
  if (!succeeded) {
    const char msg[] = "Failed to generate minidump.";
    WriteLog(msg, sizeof(msg) - 1);
    return false;
  }

  DCHECK(!minidump.IsFD());

  BreakpadInfo info = {0};
  info.filename = minidump.path();
  info.fd = minidump.fd();
  info.process_type = "shell";
  info.process_type_length = 7;
  info.distro = base::g_linux_distro;
  info.distro_length = my_strlen(base::g_linux_distro);
  info.process_start_time = g_process_start_time;
  info.oom_size = base::g_oom_size;
  info.pid = g_pid;
  info.crash_keys = g_crash_keys;
  HandleCrashDump(info);
#if defined(OS_ANDROID)
  return FinalizeCrashDoneAndroid();
#else
  return true;
#endif
}

// Wrapper function, do not add more code here.
bool CrashDoneNoUpload(const MinidumpDescriptor& minidump,
                       void* context,
                       bool succeeded) {
  return CrashDone(minidump, succeeded);
}

void EnableCrashDumping(const base::FilePath& dumps_path) {
  g_is_crash_reporter_enabled = true;

  DCHECK(!g_breakpad);
  DCHECK(base::CreateDirectoryAndGetError(dumps_path, nullptr));
  MinidumpDescriptor minidump_descriptor(dumps_path.value());
  minidump_descriptor.set_size_limit(kMaxMinidumpFileSize);
  g_breakpad = new ExceptionHandler(
      minidump_descriptor, nullptr, CrashDoneNoUpload, nullptr,
      true,  // Install handlers.
      -1);   // Server file descriptor. -1 for in-process.
}

void SetCrashKeyValue(const base::StringPiece& key,
                      const base::StringPiece& value) {
  g_crash_keys->SetKeyValue(key.data(), value.data());
}

void ClearCrashKey(const base::StringPiece& key) {
  g_crash_keys->RemoveKey(key.data());
}

void RegisterCrashKeys() {
  // TODO(qsr): Is there any key to register here?
}

void InitCrashKeys() {
  g_crash_keys = new CrashKeyStorage;
  RegisterCrashKeys();
  base::debug::SetCrashKeyReportingFunctions(&SetCrashKeyValue, &ClearCrashKey);
}

// Miscellaneous initialization functions to call after Breakpad has been
// enabled.
void PostEnableBreakpadInitialization() {
  SetProcessStartTime();
  g_pid = getpid();

  base::debug::SetDumpWithoutCrashingFunction(&DumpProcess);
}

}  // namespace

void InitCrashReporter(const base::FilePath& dumps_path) {
#if defined(OS_ANDROID)
  // This will guarantee that the BuildInfo has been initialized and subsequent
  // calls will not require memory allocation.
  base::android::BuildInfo::GetInstance();
#endif

  InitCrashKeys();
  EnableCrashDumping(dumps_path);

  PostEnableBreakpadInitialization();
}

bool IsCrashReporterEnabled() {
  return g_is_crash_reporter_enabled;
}

}  // namespace breakpad
