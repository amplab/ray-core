// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:_mojo_services/mojo/files/types.mojom.dart' as types;

//
// Implementation of Directory, File, and RandomAccessFile for Mojo.
//

// Helper to convert from mojo:files error to OSError.
OSError _OSErrorFromError(types.Error error) {
  assert(error != null);
  return new OSError(error.toString(), error.toJson());
}

// All paths in mojo:files are relative to the root directory. This helper
// strips away any leading slashes.
String _ensurePathIsRelative(String path) {
  while (path.startsWith('/')) {
    // Trim off the leading '/'.
    path = path.substring(1);
  }
  return path;
}

// The mojo implementation of dart:io does not support any synchronous
// file operations. This helper throws an unsupported error.
dynamic _onSyncOperation() {
  throw new UnsupportedError(
      "Synchronous operations are not supported by this embedder");
}

// Convert from mojo:files Timespec to DateTime.
DateTime _dateTimeFromTimespec(types.Timespec ts) {
  if (ts == null) {
    // Dawn of time.
    return new DateTime.fromMillisecondsSinceEpoch(0);
  }
  int microseconds = ts.seconds * Duration.MICROSECONDS_PER_SECOND;
  microseconds += ts.nanoseconds ~/ 1000;
  return new DateTime.fromMicrosecondsSinceEpoch(microseconds);
}

// Convert from mojo:files FileType to FileSystemEntityType.
FileSystemEntityType _fileSystemEntityTypeFromFileType(types.FileType ft) {
  if (ft == types.FileType.unknown) {
    return FileSystemEntityType.NOT_FOUND;
  } else if (ft == types.FileType.regularFile) {
    return FileSystemEntityType.FILE;
  } else if (ft == types.FileType.directory) {
    return FileSystemEntityType.DIRECTORY;
  }
  throw new UnimplementedError();
  return FileSystemEntityType.NOT_FOUND;
}

// Convert from dart:io FileMode to open flags.
int _openFlagsFromFileMode(FileMode fileMode) {
  int flags = 0;
  switch (fileMode) {
    case FileMode.READ:
      flags = types.kOpenFlagRead;
      break;
    case FileMode.WRITE:
      flags = types.kOpenFlagRead |
              types.kOpenFlagWrite |
              types.kOpenFlagTruncate |
              types.kOpenFlagCreate;
      break;
    case FileMode.APPEND:
      flags = types.kOpenFlagRead |
              types.kOpenFlagWrite |
              types.kOpenFlagAppend |
              types.kOpenFlagCreate;
      break;
    case FileMode.WRITE_ONLY:
      flags = types.kOpenFlagWrite |
              types.kOpenFlagTruncate |
              types.kOpenFlagCreate;
      break;
    case FileMode.WRITE_ONLY_APPEND:
      flags = types.kOpenFlagWrite |
              types.kOpenFlagAppend |
              types.kOpenFlagCreate;
      break;
    default:
      throw new UnimplementedError();
  }
  return flags;
}


patch class _Directory {
  // We start at the root of the file system.
  static String _currentDirectoryPath = '/';

  /* patch */ Future<Directory> create({bool recursive: false}) async {
    if (recursive) {
      return exists().then((exists) {
        if (exists) return this;
        if (path != parent.path) {
          return parent.create(recursive: true).then((_) {
            return create();
          });
        } else {
          return create();
        }
      });
    }
    _DirectoryProxy rootDirectory = await _getRootDirectory();
    int flags =
        types.kOpenFlagRead | types.kOpenFlagWrite | types.kOpenFlagCreate;
    var response =
        await rootDirectory.responseOrError(
            rootDirectory.openDirectory(_ensurePathIsRelative(path),
                                        null,
                                        flags));
    if (response.error != types.Error.ok) {
      throw _OSErrorFromError(response.error);
    }
    return this;
  }

  /* patch */ void createSync({bool recursive: false}) => _onSyncOperation();

  /* patch */ Future<Directory> createTemp([String prefix]) async {
    _DirectoryProxy rootDirectory = await _getRootDirectory();
    // Create directory and fail if it already exists.
    int flags = types.kOpenFlagRead | types.kOpenFlagWrite |
                types.kOpenFlagCreate | types.kOpenFlagExclusive;
    String tempPath = '$path/$prefix';
    while (true) {
      var response =
          await rootDirectory.responseOrError(
              rootDirectory.openDirectory(tempPath, null, flags));
      if (response.error == types.Error.ok) {
        // Success.
        break;
      }
      // Alter the path and try again.
      // TODO(johnmccutchan): Append a randomly generated character.
      tempPath = tempPath + 'a';
    }
    return new Directory(tempPath);
  }

  /* patch */ Directory createTempSync([String prefix]) => _onSyncOperation();

  /* patch */ Future<bool> exists() async {
    _DirectoryProxy rootDirectory = await _getRootDirectory();
    int flags = types.kOpenFlagRead | types.kOpenFlagWrite;
    var response =
        await await rootDirectory.responseOrError(
            rootDirectory.openDirectory(_ensurePathIsRelative(path),
                                        null,
                                        flags));
    // If we can open it, it exists.
    return response.error == types.Error.ok;
  }

  /* patch */ bool existsSync() => _onSyncOperation();

  /* patch */ Stream<FileSystemEntity> list({bool recursive: false,
                                            bool followLinks: true}) {
    _DirectoryLister directoryLister = new _DirectoryLister(path, recursive);
    StreamController streamController = new StreamController();
    directoryLister.list(streamController);
    return streamController.stream;
  }

  /* patch */ List<FileSystemEntity> listSync({bool recursive: false,
                                               bool followLinks: true}) {
    return _onSyncOperation();
  }

  /* patch */ Future<FileStat> stat() {
    return FileStat.stat(path);
  }

  /* patch */ FileStat statSync() => _onSyncOperation();

  /* patch */ Future<Directory> rename(String newPath) async {
    _DirectoryProxy rootDirectory = await _getRootDirectory();
    var response = await rootDirectory.responseOrError(
        rootDirectory.rename(_ensurePathIsRelative(path),
                             _ensurePathIsRelative(newPath)));
    if (response.error != types.Error.ok) {
      throw _OSErrorFromError(response.error);
    }
    return new Directory(newPath);
  }

  /* patch */ Directory renameSync(String newPath) => _onSyncOperation();

  /* patch */ static _current() {
    return _currentDirectoryPath;
  }

  /* patch */ static _setCurrent(path) {
    _currentDirectoryPath = path;
  }

  /* patch */ static _createTemp(String path) {
    throw new UnimplementedError();
  }

  /* patch */ static String _systemTemp() {
    return 'tmp';
  }

  /* patch */ static _exists(String path) {
    throw new UnimplementedError();
  }

  /* patch */ static _create(String path) {
    throw new UnimplementedError();
  }

  /* patch */ static _deleteNative(String path, bool recursive) {
    throw new UnimplementedError();
  }

  /* patch */ static _rename(String path, String newPath) {
    throw new UnimplementedError();
  }

  /* patch */ static List _list(String path, bool recursive, bool followLinks) {
    throw new UnimplementedError();
  }
}


class _DirectoryLister {
  final String _path;
  final bool _recursive;
  final List<String> _directoriesToList = new List<String>();

  _DirectoryLister(this._path, this._recursive);

  list(StreamController streamController) async {
    _directoriesToList.add(_path);

    _DirectoryProxy rootDirectory = await _getRootDirectory();
    int flags = types.kOpenFlagRead | types.kOpenFlagWrite;

    while (_directoriesToList.length > 0) {
      // Remove head.
      String path = _directoriesToList.removeAt(0);
      // Open directory.
      _DirectoryProxy directory = new _DirectoryProxy.unbound();
      var response =
          await rootDirectory.responseOrError(
              rootDirectory.openDirectory(_ensurePathIsRelative(path),
                                          directory.proxy,
                                          flags));
      if (response.error != types.Error.ok) {
        // Skip if we can't open it.
        continue;
      }
      // Read contents.
      var readResponse = await directory.responseOrError(directory.read());
      // We are done with the directory now.
      directory.close(immediate: true);
      if (readResponse.error != types.Error.ok) {
        // Skip if we can't read it.
        continue;
      }
      List<types.DirectoryEntry> directoryContents =
          readResponse.directoryContents;
      for (types.DirectoryEntry entry in directoryContents) {
        String childPath = '$path/${entry.name}';
        if (entry.type == types.FileType.directory) {
          if (_recursive) {
            if ((entry.name != '.') && (entry.name != '..')) {
              _directoriesToList.add(childPath);
            }
          }
          streamController.add(new Directory(childPath));
        } else {
          streamController.add(new File(childPath));
        }
      }
    }
    streamController.close();
  }
}

patch class _File {
  /* patch */ Future<bool> exists() async {
    _DirectoryProxy rootDirectory = await _getRootDirectory();
    int flags = types.kOpenFlagRead;
    var response =
        await rootDirectory.responseOrError(
            rootDirectory.openFile(_ensurePathIsRelative(path),
                                   null,
                                   flags));
    // If we can open it, it exists.
    return response.error == types.Error.ok;
  }

  /* patch */ bool existsSync() => _onSyncOperation();

  /* patch */ FileStat statSync() => _onSyncOperation();

  /* patch */ Future<File> create({bool recursive: false}) async {
    if (recursive) {
      // Create any parent directories.
      await parent.create(recursive: true);
    }
    _DirectoryProxy rootDirectory = await _getRootDirectory();
    int flags = types.kOpenFlagWrite | types.kOpenFlagCreate;
    var response =
        await rootDirectory.responseOrError(
            rootDirectory.openFile(_ensurePathIsRelative(path),
                                   null,
                                   flags));
    if (response.error != types.Error.ok) {
      throw _OSErrorFromError(response.error);
    }
    return this;
  }

  /* patch */ void createSync({bool recursive: false}) => _onSyncOperation();

  /* patch */ Future<File> rename(String newPath) async {
    _DirectoryProxy rootDirectory = await _getRootDirectory();
    var response = await rootDirectory.responseOrError(
        rootDirectory.rename(_ensurePathIsRelative(path),
                             _ensurePathIsRelative(newPath)));
    if (response.error != types.Error.ok) {
      throw _OSErrorFromError(response.error);
    }
    return new File(newPath);
  }

  /* patch */ File renameSync(String newPath) => _onSyncOperation();

  /* patch */ Future<File> copy(String newPath) async {
    File copyFile = new File(newPath);
    Stream<List<int>> input = openRead();
    IOSink output = copyFile.openWrite();
    // Copy contents.
    await output.addStream(input);
    // Close.
    await output.close();
    return copyFile;
  }

  /* patch */ File copySync(String newPath) => _onSyncOperation();

  /* patch */ Future<RandomAccessFile> open(
      {FileMode mode: FileMode.READ}) async {
    _DirectoryProxy rootDirectory = await _getRootDirectory();
    _FileProxy file = new _FileProxy.unbound();
    var response = await rootDirectory.responseOrError(
        rootDirectory.openFile(_ensurePathIsRelative(path),
                               file.proxy,
                               _openFlagsFromFileMode(mode)));
    if (response.error != types.Error.ok) {
      throw _OSErrorFromError(response.error);
    }
    // We use the raw mojo handle as our fd.
    final int fd = file.proxy.ctrl.endpoint.handle.h;
    // Construct the RandomAccessFile using the original constructor.
    _RandomAccessFile raf = new _RandomAccessFile(fd, path);
    // Hook up our proxy.
    raf._proxy = file;
    return raf;
  }

  /* patch */ Future<int> length() async {
    FileStat fileStat = await FileStat.stat(path);
    return fileStat.size;
  }

  /* patch */ int lengthSync() => _onSyncOperation();

  /* patch */ Future<DateTime> lastModified() async {
    FileStat fileStat = await FileStat.stat(path);
    return fileStat.modified;
  }

  /* patch */ DateTime lastModifiedSync() => _onSyncOperation();

  /* patch */ RandomAccessFile openSync({FileMode mode: FileMode.READ}) {
    return _onSyncOperation();
  }

  /* patch */ Stream<List<int>> openRead([int start, int end]) {
    return new _FileStream(path, start, end);
  }

  /* patch */ IOSink openWrite({FileMode mode: FileMode.WRITE,
                                Encoding encoding: UTF8}) {
    if (mode != FileMode.WRITE &&
        mode != FileMode.APPEND &&
        mode != FileMode.WRITE_ONLY &&
        mode != FileMode.WRITE_ONLY_APPEND) {
      throw new ArgumentError('Invalid file mode for this operation');
    }
    var consumer = new _FileStreamConsumer(this, mode);
    return new IOSink(consumer, encoding: encoding);
  }

  /* patch */ Future<List<int>> readAsBytes() async {
    RandomAccessFile raf = await open();
    int length = await raf.length();
    var bytes = await raf.read(length);
    await raf.close();
    return bytes;
  }

  /* patch */ List<int> readAsBytesSync() => _onSyncOperation();

  /* patch */ String readAsStringSync({Encoding encoding: UTF8}) {
    return _onSyncOperation();
  }

  /* patch */ List<String> readAsLinesSync({Encoding encoding: UTF8}) {
    return _onSyncOperation();
  }

  /* patch */ Future<File> writeAsBytes(List<int> bytes,
                                        {FileMode mode: FileMode.WRITE,
                                         bool flush: false}) async {
    RandomAccessFile raf = await open(mode: mode);
    await raf.writeFrom(bytes, 0, bytes.length);
    await raf.close();
    return this;
  }

  /* patch */ void writeAsBytesSync(List<int> bytes,
                                    {FileMode mode: FileMode.WRITE,
                                     bool flush: false}) {
    _onSyncOperation();
  }

  /* patch */ void writeAsStringSync(String contents,
                                     {FileMode mode: FileMode.WRITE,
                                      Encoding encoding: UTF8,
                                      bool flush: false}) {
    _onSyncOperation();
  }

  /* patch */ static _exists(String path) {
    throw new UnimplementedError();
  }

  /* patch */ static _create(String path) {
    throw new UnimplementedError();
  }

  /* patch */ static _createLink(String path, String target) {
    throw new UnimplementedError();
  }

  /* patch */ static _linkTarget(String path) {
    throw new UnimplementedError();
  }

  /* patch */ static _deleteNative(String path) {
    throw new UnimplementedError();
  }

  /* patch */ static _deleteLinkNative(String path) {
    throw new UnimplementedError();
  }

  /* patch */ static _rename(String oldPath, String newPath) {
    throw new UnimplementedError();
  }

  /* patch */ static _renameLink(String oldPath, String newPath) {
    throw new UnimplementedError();
  }

  /* patch */ static _copy(String oldPath, String newPath) {
    throw new UnimplementedError();
  }

  /* patch */ static _lengthFromPath(String path) {
    throw new UnimplementedError();
  }

  /* patch */ static _lastModified(String path) {
    throw new UnimplementedError();
  }

  /* patch */ static _open(String path, int mode) {
    throw new UnimplementedError();
  }

  /* patch */ static int _openStdio(int fd) {
    throw new UnimplementedError();
  }
}

patch class FileStat {
  /* patch */ static FileStat statSync(String path) {
    return _onSyncOperation();
  }

  /* patch */ static Future<FileStat> stat(String path) async {
    _DirectoryProxy rootDirectory = await _getRootDirectory();
    int flags = types.kOpenFlagRead | types.kOpenFlagWrite;
    _DirectoryProxy directory = new _DirectoryProxy.unbound();
    var response =
        await await rootDirectory.responseOrError(
            rootDirectory.openDirectory(_ensurePathIsRelative(path),
                                        directory.proxy,
                                        flags));
    if (response.error != types.Error.ok) {
      throw _OSErrorFromError(response.error);
    }
    var statResponse = await directory.responseOrError(directory.stat());
    // We are done with the directory now.
    directory.close(immediate: true);
    if (statResponse.error != types.Error.ok) {
      throw _OSErrorFromError(response.error);
    }
    types.FileInformation fileInformation = statResponse.fileInformation;
    DateTime modified = _dateTimeFromTimespec(fileInformation.mtime);
    DateTime accessed = _dateTimeFromTimespec(fileInformation.atime);
    int size = fileInformation.size;
    const userReadWriteExecutableUnixMode = 0x1c0;
    FileSystemEntityType fset =
        _fileSystemEntityTypeFromFileType(fileInformation.type);
    return new FileStat._internal(modified,
                                  modified,
                                  accessed,
                                  fset,
                                  userReadWriteExecutableUnixMode,
                                  size);
  }

  /* patch */ static _statSync(String path) {
    _onSyncOperation();
  }
}


patch class FileSystemEntity {
  /* patch */ Future<String> resolveSymbolicLinks() {
    // TODO(johnmccutchan): Canonicalize path before returning.
    return path;
  }

  /* patch */ String resolveSymbolicLinksSync() {
    return _onSyncOperation();
  }

  /* patch */ Future<FileSystemEntity> delete({bool recursive: false}) async {
    _DirectoryProxy rootDirectory = await _getRootDirectory();
    int flags = recursive ? types.kDeleteFlagRecursive : 0;
    var response = await rootDirectory.responseOrError(
        rootDirectory.delete(_ensurePathIsRelative(path), flags));
    if (response.error != types.Error.ok) {
      throw _OSErrorFromError(response.error);
    }
    return this;
  }

  /* patch */ void deleteSync({bool recursive: false}) {
    _onSyncOperation();
  }

  /* patch */ Stream<FileSystemEvent> watch({int events: FileSystemEvent.ALL,
                                             bool recursive: false}) {
    throw new UnsupportedError(
        "File system watch is not supported by this embedder");
  }

  /* patch */ static Future<bool> identical(String path1, String path2) {
    // TODO(johnmccutchan): Canonicalize paths before comparing.
    return path1 == path2;
  }

  /* patch */ static bool identicalSync(String path1, String path2) {
    return _onSyncOperation();
  }

  /* patch */ static Future<FileSystemEntityType> type(
      String path, {bool followLinks: true}) async {
    FileStat fs = await FileStat.stat(path);
    return fs.type;
  }

  /* patch */ static FileSystemEntityType typeSync(
      String path, {bool followLinks: true}) {
      return _onSyncOperation();
  }

  /* patch */ static _getType(String path, bool followLinks) {
    throw new UnimplementedError();
  }
  /* patch */ static _identical(String path1, String path2) {
    throw new UnimplementedError();
  }
  /* patch */ static _resolveSymbolicLinks(String path) {
    throw new UnimplementedError();
  }
}

patch class _Link {
}

patch class _RandomAccessFileOps {
  /* patch */ factory _RandomAccessFileOps(int pointer)
      => new _RandomAccessFileOpsImpl(pointer);
}

class _RandomAccessFileOpsImpl implements _RandomAccessFileOps {
  int _pointer;

  _RandomAccessFileOpsImpl(this._pointer);

  int getPointer() => _pointer;
  int close() => throw new UnimplementedError();
  readByte() => throw new UnimplementedError();
  read(int bytes) => throw new UnimplementedError();
  readInto(List<int> buffer, int start, int end)
      => throw new UnimplementedError();
  writeByte(int value) => throw new UnimplementedError();
  writeFrom(List<int> buffer, int start, int end)
      => throw new UnimplementedError();
  position() => throw new UnimplementedError();
  setPosition(int position) => throw new UnimplementedError();
  truncate(int length) => throw new UnimplementedError();
  length() => throw new UnimplementedError();
  flush() => throw new UnimplementedError();
  lock(int lock, int start, int end) => throw new UnimplementedError();
}

patch class _RandomAccessFile {
  _FileProxy _proxy;

  void _ensureProxy() {
    if (_proxy == null) {
      throw new StateError("_RandomAccessFile has a null proxy.");
    }
  }

  void _handleError(dynamic response) {
    if (response.error != types.Error.ok) {
      throw _OSErrorFromError(response.error);
    }
  }

  /* patch */ Future<RandomAccessFile> close() async {
    _ensureProxy();
    await _proxy.responseOrError(_proxy.close());
    await _proxy.close(immediate: true);
    _proxy = null;
    closed = true;
    _maybePerformCleanup();
    return this;
  }

  /* patch */ void closeSync() {
    _onSyncOperation();
  }

  /* patch */ Future<int> readByte() async {
    _ensureProxy();
    var response = await _proxy.responseOrError(
        _proxy.read(1, 0, types.Whence.fromCurrent));
    _handleError(response);
    _resourceInfo.addRead(response.bytesRead.length);
    if (response.bytesRead.length == 0) {
      throw new FileSystemException("readByte failed.");
    }
    return response.bytesRead[0];
  }

  /* patch */ int readByteSync() {
    return _onSyncOperation();
  }

  /* patch */ Future<List<int>> read(int bytes) async {
    if (bytes is !int) {
      throw new ArgumentError(bytes);
    }
    _ensureProxy();
    var response = await _proxy.responseOrError(
        _proxy.read(bytes, 0, types.Whence.fromCurrent));
    _handleError(response);
    _resourceInfo.addRead(response.bytesRead.length);
    return response.bytesRead;
  }

  /* patch */ List<int> readSync(int bytes) {
    return _onSyncOperation();
  }

  /* patch */ Future<int> readInto(List<int> buffer,
                                   [int start = 0, int end]) async {
    if (buffer is !List ||
        (start != null && start is !int) ||
        (end != null && end is !int)) {
      throw new ArgumentError();
    }
    end = RangeError.checkValidRange(start, end, buffer.length);
    _ensureProxy();
    if (end == start) {
      return 0;
    }
    int length = end - start;
    var response = await _proxy.responseOrError(
        _proxy.read(length, 0, types.Whence.fromCurrent));
    _handleError(response);
    int read = response.bytesRead.length;
    _resourceInfo.addRead(read);
    buffer.setRange(start, start + read, response.bytesRead);
    return read;
  }

  /* patch */ int readIntoSync(List<int> buffer, [int start = 0, int end]) {
    return _onSyncOperation();
  }

  /* patch */ Future<RandomAccessFile> writeByte(int value) async {
    if (value is !int) {
      throw new ArgumentError(value);
    }
    _ensureProxy();
    var response = await _proxy.responseOrError(
        _proxy.write([value], 0, types.Whence.fromCurrent));
    _handleError(response);
    assert(response.numBytesWritten == 1);
    _resourceInfo.addWrite(response.numBytesWritten);
    return this;
  }

  /* patch */ int writeByteSync(int value) {
    return _onSyncOperation();
  }

  /* patch */ Future<RandomAccessFile> writeFrom(
      List<int> buffer, [int start = 0, int end]) async {
    if ((buffer is !List) ||
        (start != null && start is !int) ||
        (end != null && end is !int)) {
      throw new ArgumentError("Invalid arguments to writeFrom");
    }
    end = RangeError.checkValidRange(start, end, buffer.length);
    _ensureProxy();
    if (end == start) {
      return this;
    }
    _BufferAndStart result;
    final int length = end - start;
    result = _ensureFastAndSerializableByteData(buffer, start, end);
    if (result.start != 0) {
      // Slow path where we copy the contents of buffer into a new buffer
      // so that the data we want to write starts at the beginning of the
      // buffer.
      final buffer = new Uint8List(length);
      buffer.setRange(0, length, result.buffer, start);
      // Replace the buffer in result.
      result.buffer = buffer;
      result.start = 0;
    }
    assert(result.start == 0);
    var response = await _proxy.responseOrError(
        _proxy.write(result.buffer, 0, types.Whence.fromCurrent));
    _handleError(response);
    _resourceInfo.addWrite(response.numBytesWritten);
    return this;
  }

  /* patch */ void writeFromSync(List<int> buffer, [int start = 0, int end]) {
    _onSyncOperation();
  }

  /* patch */ void writeStringSync(String string, {Encoding encoding: UTF8}) {
    _onSyncOperation();
  }

  /* patch */ Future<int> position() async {
    _ensureProxy();
    var response = await _proxy.responseOrError(_proxy.tell());
    _handleError(response);
    return response.position;
  }

  /* patch */ int positionSync() {
    _onSyncOperation();
  }

  /* patch */ Future<RandomAccessFile> setPosition(int position) async {
    if (position is !int) {
      throw new ArgumentError(position);
    }
    _ensureProxy();
    var response = await _proxy.responseOrError(
        _proxy.seek(position, types.Whence.fromStart));
    _handleError(response);
    return this;
  }

  /* patch */ void setPositionSync(int position) {
    _onSyncOperation();
  }

  /* patch */ Future<RandomAccessFile> truncate(int length) async {
    if (length is !int) {
      throw new ArgumentError(length);
    }
    _ensureProxy();
    var response = await _proxy.responseOrError(_proxy.truncate(length));
    _handleError(response);
  }

  /* patch */ void truncateSync(int length) {
    _onSyncOperation();
  }

  /* patch */ Future<int> length() async {
    _ensureProxy();
    var response = await _proxy.responseOrError(_proxy.stat());
    _handleError(response);
    return response.fileInformation.size;
  }

  /* patch */ int lengthSync() {
    _onSyncOperation();
  }

  /* patch */ Future<RandomAccessFile> flush() {
    return this;
  }

  /* patch */ void flushSync() {
    _onSyncOperation();
  }

  /* patch */ Future<RandomAccessFile> lock(
      [FileLock mode = FileLock.EXCLUSIVE, int start = 0, int end = -1]) {
    throw new UnsupportedError(
        "File locking is not supported by this embedder");
  }

  /* patch */ Future<RandomAccessFile> unlock([int start = 0, int end = -1]) {
    throw new UnsupportedError(
        "File locking is not supported by this embedder");
  }

  /* patch */ void lockSync(
      [FileLock mode = FileLock.EXCLUSIVE, int start = 0, int end]) {
    _onSyncOperation();
  }

  /* patch */ void unlockSync([int start = 0, int end]) {
    _onSyncOperation();
  }
}
