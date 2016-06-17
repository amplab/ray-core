// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:nativewrappers';

import 'dart:_mojo/application.dart';
import 'dart:_mojo/bindings.dart';
import 'dart:_mojo/core.dart';
import 'dart:_mojo/mojo/network_error.mojom.dart';
import 'dart:_mojo_services/mojo/host_resolver.mojom.dart';
import 'dart:_mojo_services/mojo/net_address.mojom.dart';
import 'dart:_mojo_services/mojo/network_service.mojom.dart';
import 'dart:_mojo_services/mojo/tcp_bound_socket.mojom.dart';
import 'dart:_mojo_services/mojo/tcp_connected_socket.mojom.dart';
import 'dart:_mojo_services/mojo/tcp_server_socket.mojom.dart';
import 'dart:_mojo_services/mojo/files/file.mojom.dart';
import 'dart:_mojo_services/mojo/files/files.mojom.dart';
import 'dart:_mojo_services/mojo/files/directory.mojom.dart';
import 'dart:_mojo_services/mojo/files/ioctl.mojom.dart';
import 'dart:_mojo_services/mojo/files/types.mojom.dart' as types;

// TODO(zra): Investigate the runtime cost of using these wrapper classes.
// Port to use the callback API where necessary to recover performance.
class _NetworkServiceProxy extends FuturizedProxy<NetworkServiceProxy> {
  Map<Symbol, Function> _mojoMethods;
  NetworkServiceProxy _nsProxy;

  _NetworkServiceProxy(NetworkServiceProxy p) : super(p) {
    _mojoMethods = <Symbol, Function>{
      #createTcpBoundSocket: p.createTcpBoundSocket,
      #createTcpConnectedSocket: p.createTcpConnectedSocket,
      #createHttpServer: p.createHttpServer,
    };
    _nsProxy = p;
  }
  Map<Symbol, Function> get mojoMethods => _mojoMethods;

  Function get createUrlLoader => _nsProxy.createUrlLoader;
  Function get getCookieStore => _nsProxy.getCookieStore;
  Function get createWebSocket => _nsProxy.createWebSocket;
  Function get createUdpSocket => _nsProxy.createUdpSocket;
  Function get registerUrlLoaderInterceptor =>
      _nsProxy.registerUrlLoaderInterceptor;
  Function get createHostResolver => _nsProxy.createHostResolver;

  static final Map<Symbol, Function> _mojoResponses = {
    #createTcpBoundSocket:
        new NetworkServiceCreateTcpBoundSocketResponseParams#init,
    #createTcpConnectedSocket:
        new NetworkServiceCreateTcpConnectedSocketResponseParams#init,
    #createHttpServer:
        new NetworkServiceCreateHttpServerResponseParams#init,
  };
  Map<Symbol, Function> get mojoResponses => _mojoResponses;
}

class _HostResolverProxy extends FuturizedProxy<HostResolverProxy> {
  Map<Symbol, Function> _mojoMethods;

  _HostResolverProxy(HostResolverProxy proxy) : super(proxy) {
    _mojoMethods = <Symbol, Function>{
      #getHostAddresses: proxy.getHostAddresses,
    };
  }
  Map<Symbol, Function> get mojoMethods => _mojoMethods;

  _HostResolverProxy.unbound() : this(new HostResolverProxy.unbound());

  static final Map<Symbol, Function> _mojoResponses = {
    #getHostAddresses: new HostResolverGetHostAddressesResponseParams#init,
  };
  Map<Symbol, Function> get mojoResponses => _mojoResponses;
}

class _TcpBoundSocketProxy extends FuturizedProxy<TcpBoundSocketProxy> {
  Map<Symbol, Function> _mojoMethods;

  _TcpBoundSocketProxy(TcpBoundSocketProxy proxy) : super(proxy) {
    _mojoMethods = <Symbol, Function>{
      #startListening: proxy.startListening,
      #connect: proxy.connect,
    };
  }
  Map<Symbol, Function> get mojoMethods => _mojoMethods;

  _TcpBoundSocketProxy.unbound() : this(new TcpBoundSocketProxy.unbound());

  static final Map<Symbol, Function> _mojoResponses = {
    #startListening: new TcpBoundSocketStartListeningResponseParams#init,
    #connect: new TcpBoundSocketConnectResponseParams#init,
  };
  Map<Symbol, Function> get mojoResponses => _mojoResponses;
}

class _TcpServerSocketProxy extends FuturizedProxy<TcpServerSocketProxy> {
  Map<Symbol, Function> _mojoMethods;

  _TcpServerSocketProxy(TcpServerSocketProxy proxy) : super(proxy) {
    _mojoMethods = <Symbol, Function>{
      #accept: proxy.accept,
    };
  }
  Map<Symbol, Function> get mojoMethods => _mojoMethods;

  _TcpServerSocketProxy.unbound() : this(new TcpServerSocketProxy.unbound());

  static final Map<Symbol, Function> _mojoResponses = {
    #accept: new TcpServerSocketAcceptResponseParams#init,
  };
  Map<Symbol, Function> get mojoResponses => _mojoResponses;
}

class _FilesProxy extends FuturizedProxy<FilesProxy> {
  Map<Symbol, Function> _mojoMethods;

  _FilesProxy(FilesProxy proxy) : super(proxy) {
    _mojoMethods = <Symbol, Function>{
      #openFileSystem: proxy.openFileSystem,
    };
  }
  Map<Symbol, Function> get mojoMethods => _mojoMethods;

  static final Map<Symbol, Function> _mojoResponses = {
    #openFileSystem: new FilesOpenFileSystemResponseParams#init,
  };
  Map<Symbol, Function> get mojoResponses => _mojoResponses;
}

class _FileProxy extends FuturizedProxy<FileProxy> {
  Map<Symbol, Function> _mojoMethods;

  _FileProxy(FileProxy proxy) : super(proxy) {
    _mojoMethods = <Symbol, Function>{
      #close_: proxy.close_,
      #read: proxy.read,
      #write: proxy.write,
      #readToStream: proxy.readToStream,
      #writeFromStream: proxy.writeFromStream,
      #tell: proxy.tell,
      #seek: proxy.seek,
      #stat: proxy.stat,
      #truncate: proxy.truncate,
      #touch: proxy.touch,
      #dup: proxy.dup,
      #repoen: proxy.reopen,
      #asBuffer: proxy.asBuffer,
      #ioctl: proxy.ioctl,
    };
  }
  Map<Symbol, Function> get mojoMethods => _mojoMethods;

  _FileProxy.unbound() : this(new FileProxy.unbound());

  static final Map<Symbol, Function> _mojoResponses = {
    #close_: new FileCloseResponseParams#init,
    #read: new FileReadResponseParams#init,
    #write: new FileWriteResponseParams#init,
    #readToStream: new FileReadToStreamResponseParams#init,
    #writeFromStream: new FileWriteFromStreamResponseParams#init,
    #tell: new FileTellResponseParams#init,
    #seek: new FileSeekResponseParams#init,
    #stat: new FileStatResponseParams#init,
    #truncate: new FileTruncateResponseParams#init,
    #touch: new FileTouchResponseParams#init,
    #dup: new FileDupResponseParams#init,
    #repoen: new FileReopenResponseParams#init,
    #asBuffer: new FileAsBufferResponseParams#init,
    #ioctl: new FileIoctlResponseParams#init,
  };
  Map<Symbol, Function> get mojoResponses => _mojoResponses;
}

class _DirectoryProxy extends FuturizedProxy<DirectoryProxy> {
  Map<Symbol, Function> _mojoMethods;

  _DirectoryProxy(DirectoryProxy proxy) : super(proxy) {
    _mojoMethods = <Symbol, Function>{
      #read: proxy.read,
      #stat: proxy.stat,
      #touch: proxy.touch,
      #openFile: proxy.openFile,
      #openDirectory: proxy.openDirectory,
      #rename: proxy.rename,
      #delete: proxy.delete,
    };
  }
  Map<Symbol, Function> get mojoMethods => _mojoMethods;

  _DirectoryProxy.unbound() : this(new DirectoryProxy.unbound());

  static final Map<Symbol, Function> _mojoResponses = {
    #read: new DirectoryReadResponseParams#init,
    #stat: new DirectoryStatResponseParams#init,
    #touch: new DirectoryTouchResponseParams#init,
    #openFile: new DirectoryOpenFileResponseParams#init,
    #openDirectory: new DirectoryOpenDirectoryResponseParams#init,
    #rename: new DirectoryRenameResponseParams#init,
    #delete: new DirectoryDeleteResponseParams#init,
  };
  Map<Symbol, Function> get mojoResponses => _mojoResponses;
}

// When developing, set fileSystemDeveloper to true and the file system will
// persist under ~/MojoAppPersistentCaches/.
const bool fileSystemDeveloper = false;
const String fileSystemName =
    fileSystemDeveloper ? 'app_persistent_cache' : null;

// System temp path relative to the root directory.
const String systemTempPath = 'tmp';

//
// Mojo objects and helper functions used by the 'dart:io' library.
//
int _networkServiceHandle;
int _filesServiceHandle;
_NetworkServiceProxy _networkService;
_HostResolverProxy _hostResolver;
_FilesProxy _files;
_DirectoryProxy _rootDirectory;
_DirectoryProxy _systemTempDirectory;

void _initialize(
    int networkServiceHandle, int filesServiceHandle, String scriptPath) {
  if (networkServiceHandle != MojoHandle.INVALID) {
    _networkServiceHandle = networkServiceHandle;
  }
  if (filesServiceHandle != MojoHandle.INVALID) {
    _filesServiceHandle = filesServiceHandle;
  }
  // TODO(floitsch): do this lazily once _Platform.script is a getter.
  _Platform.script = Uri.parse(scriptPath);
}

void _shutdown() {
  if (_networkServiceHandle != null) {
    // Network service proxies were never initialized. Create a handle
    // and close it.
    var handle = new MojoHandle(_networkServiceHandle);
    _networkServiceHandle = null;
    handle.close();
  }
  if (_filesServiceHandle != null) {
    // File system proxies were never initialized. Create a handle and close it.
    var handle = new MojoHandle(_filesServiceHandle);
    _filesServiceHandle = null;
    handle.close();
  }
  _closeProxies();
}

/// Close any active proxies.
_closeProxies() {
  if (_networkService != null) {
    _networkService.close(immediate: true);
    _networkService = null;
  }
  if (_hostResolver != null) {
    _hostResolver.close(immediate: true);
    _hostResolver = null;
  }
  if (_files != null) {
    _files.close(immediate: true);
    _files = null;
  }
  if (_rootDirectory != null) {
    _rootDirectory.close(immediate: true);
    _rootDirectory = null;
  }
  if (_systemTempDirectory != null) {
    _systemTempDirectory.close(immediate: true);
    _systemTempDirectory = null;
  }
}

/// Get the singleton NetworkServiceProxy.
_NetworkServiceProxy _getNetworkService() {
  if (_networkService != null) {
    return _networkService;
  }
  var networkService = new NetworkServiceProxy.fromHandle(
      new MojoHandle(_networkServiceHandle).pass());
  _networkService = new _NetworkServiceProxy(networkService);
  _networkServiceHandle = null;
  return _networkService;
}

/// Get the singleton HostResolverProxy.
_HostResolverProxy _getHostResolver() {
  if (_hostResolver != null) {
    return _hostResolver;
  }
  _NetworkServiceProxy networkService = _getNetworkService();
  if (networkService == null) {
    return null;
  }
  _hostResolver = new _HostResolverProxy.unbound();
  networkService.createHostResolver(_hostResolver.proxy);
  // Remove the host resolver's handle from the open set because it is not
  // under application control and does not affect isolate shutdown.
  _hostResolver.proxy.ctrl.endpoint.handle.pass();
  return _hostResolver;
}

/// Get the singleton FilesProxy.
_FilesProxy _getFiles() {
  if (_files != null) {
    return _files;
  }
  var files = new FilesProxy.fromHandle(
      new MojoHandle(_filesServiceHandle).pass());
  _files = new _FilesProxy(files);
  _filesServiceHandle = null;
  return _files;
}

/// Get the singleton DirectoryProxy for the root directory.
Future<_DirectoryProxy> _getRootDirectory() async {
  if (_rootDirectory != null) {
    return _rootDirectory;
  }
  _FilesProxy files = _getFiles();
  assert(files != null);
  _rootDirectory = new _DirectoryProxy.unbound();
  var response =
      await files.openFileSystem(fileSystemName, _rootDirectory.proxy);
  // Remove the root directory's handle from the open set because it is not
  // under application control and does not affect isolate shutdown.
  _rootDirectory.proxy.ctrl.endpoint.handle.pass();

  // Ensure system temporary directory exists before returning the root
  // directory.
  await _getSystemTempDirectory();
  return _rootDirectory;
}

/// Get the singleton DirectoryProxy for the system temp directory.
Future<_DirectoryProxy> _getSystemTempDirectory() async {
  if (_systemTempDirectory != null) {
    return _systempTempDirectory;
  }
  _DirectoryProxy rootDirectory = await _getRootDirectory();
  int flags = types.kOpenFlagRead |
              types.kOpenFlagWrite |
              types.kOpenFlagCreate;
  _systemTempDirectory = new _DirectoryProxy.unbound();
  var response =
      await rootDirectory.openDirectory(systemTempPath,
                                        _systemTempDirectory.proxy,
                                        flags);
  assert(response.error == types.Error.ok);
  // Remove the system temp directory's handle from the open set because it
  // is not under application control and does not affect isolate shutdown.
  _systemTempDirectory.proxy.ctrl.endpoint.handle.pass();
  return _systemTempDirectory;
}

/// Static utility methods for converting between 'dart:io' and
/// 'mojo:network_service' structs.
class _NetworkServiceCodec {
  /// Returns a string representation of an ip address encoded in [address].
  /// Supports both IPv4 and IPv6.
  static String _addressToString(List<int> address) {
    String r = '';
    if (address == null) {
      return r;
    }
    bool ipv4 = address.length == 4;
    if (ipv4) {
      for (var i = 0; i < 4; i++) {
        var digit = address[i].toString();
        var divider = (i != 3) ? '.' : '';
        r += '${digit}${divider}';
      }
    } else {
      for (var i = 0; i < 16; i += 2) {
        var first = '';
        if (address[i] != 0) {
          first = address[i].toRadixString(16).padLeft(2, '0');
        }
        var second = address[i + 1].toRadixString(16).padLeft(2, '0');
        var digit = '$first$second';
        var divider = (i != 14) ? ':' : '';
        r += '${digit}${divider}';
      }
    }
    return r;
  }

  /// Convert from [NetAddress] to [InternetAddress].
  static InternetAddress _fromNetAddress(NetAddress netAddress) {
    if (netAddress == null) {
      return null;
    }
    var address;
    if (netAddress.family == NetAddressFamily.ipv4) {
      address = netAddress.ipv4.addr;
    } else if (netAddress.family == NetAddressFamily.ipv6) {
      address = netAddress.ipv6.addr;
    } else {
      return null;
    }
    assert(address != null);
    var addressString = _addressToString(address);
    return new InternetAddress(addressString);
  }

  static int _portFromNetAddress(NetAddress netAddress) {
    if (netAddress == null) {
      return null;
    }
    if (netAddress.family == NetAddressFamily.ipv4) {
      return netAddress.ipv4.port;
    } else if (netAddress.family == NetAddressFamily.ipv6) {
      return netAddress.ipv6.port;
    } else {
      return null;
    }
  }

  /// Convert from [InternetAddress] to [NetAddress].
  static NetAddress _fromInternetAddress(InternetAddress internetAddress,
                                         [port]) {
    if (internetAddress == null) {
      return null;
    }
    var netAddress = new NetAddress();
    var rawAddress = internetAddress.rawAddress;
    if (rawAddress.length == 4) {
      netAddress.family = NetAddressFamily.ipv4;
      netAddress.ipv4 = new NetAddressIPv4();
      netAddress.ipv4.addr = new List.from(rawAddress, growable: false);
      if (port != null) {
        netAddress.ipv4.port = port;
      }
    } else {
      assert(rawAddress.length == 16);
      netAddress.family = NetAddressFamily.ipv6;
      netAddress.ipv6 = new NetAddressIPv6();
      netAddress.ipv6.addr = new List.from(rawAddress, growable: false);
      if (port != null) {
        netAddress.ipv6.port = port;
      }
    }
    return netAddress;
  }
}

/// Static utility methods for dealing with 'mojo:network_service'.
class _NetworkService {
  /// Return a [NetAddress] for localhost:port.
  static NetAddress _localhostIpv4([int port = 0]) {
    var addr = new NetAddress();
    addr.family = NetAddressFamily.ipv4;
    addr.ipv4 = new NetAddressIPv4();
    addr.ipv4.addr = [127, 0, 0, 1];
    addr.ipv4.port = port;
    return addr;
  }

  /// Return a [NetAddress] for localhost:port.
  static NetAddress _localHostIpv6([int port = 0]) {
    var addr = new NetAddress();
    addr.family = NetAddressFamily.ipv6;
    addr.ipv6 = new NetAddressIPv6();
    addr.ipv6.addr = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1];
    addr.ipv6.port = port;
    return addr;
  }

  static bool _okay(NetworkError error) {
    if (error == null) {
      return true;
    }
    return error.code == 0;
  }

  static _throwOnError(NetworkError error) {
    if (_okay(error)) {
      return;
    }
    throw new OSError(error.description, error.code);
  }
}

