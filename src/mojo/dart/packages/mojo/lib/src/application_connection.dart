// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

part of application;

typedef void ServiceFactory(core.MojoMessagePipeEndpoint endpoint);
typedef void FallbackServiceFactory(
    String interfaceName, core.MojoMessagePipeEndpoint endpoint);

class LocalServiceProvider implements ServiceProvider {
  final ApplicationConnection connection;
  ServiceProviderStub _stub;

  LocalServiceProvider(this.connection, this._stub) {
    _stub.impl = this;
    if (!_stub.ctrl.isOpen) {
      throw new core.MojoApiError("The service provider stub must be open");
    }
  }

  set onError(Function f) {
    _stub.ctrl.onError = f;
  }

  Future close({bool immediate: false}) => _stub.close(immediate: immediate);

  void connectToService_(
      String interfaceName, core.MojoMessagePipeEndpoint pipe) {
    if (connection._nameToServiceFactory.containsKey(interfaceName)) {
      connection._nameToServiceFactory[interfaceName](pipe);
      return;
    }
    if (connection.fallbackServiceFactory != null) {
      connection.fallbackServiceFactory(interfaceName, pipe);
      return;
    }
    // The specified interface isn't provided. Close the pipe so the
    // remote endpoint sees that we don't support this interface.
    pipe.close();
  }
}

/// [ApplicationConnection] encapsulates a pair of ServiceProviders that enable
/// services (interface implementations) to be provided to a remote application
/// as well as exposing services provided by a remote application.
/// [ApplicationConnection] objects are returned by
/// [Application.connectToApplication], and are passed to
/// [Application.acceptConnection].
///
/// To request a service (e.g. `Foo`) from the remote application:
///   var foo = new FooInterfaceRequest();
///   applicationConnection.requestService(foo);
///
/// To provide a service to the remote application, specify a function that
/// instantiantes a service. For example:
///   applicationConnection.provideService(
///       Foo.serviceName, (pipe) => new FooImpl(pipe));
/// or more succinctly:
///   applicationConnection.provideService(Foo.serviceName, new FooImpl#);
///
/// To handle requests for services beyond those set up with [provideService],
/// set [fallbackServiceFactory] to a function that instantiates a service as in
/// the [provideService] case, or closes the pipe.
// TODO(vtl): Once "exposed_services" is removed from Shell's
// ConnectToApplication() (and Application's AcceptConnection(), etc.), this
// class will be a bit of overkill. https://github.com/domokit/mojo/issues/762
class ApplicationConnection {
  ServiceProviderProxy remoteServiceProvider;
  LocalServiceProvider _localServiceProvider;
  final _nameToServiceFactory = new HashMap<String, ServiceFactory>();
  final _serviceDescriptions =
      new HashMap<String, service_describer.ServiceDescription>();
  FallbackServiceFactory _fallbackServiceFactory;
  core.ErrorHandler onError;

  ApplicationConnection(ServiceProviderStub stub, this.remoteServiceProvider) {
    if (stub != null) {
      _localServiceProvider = new LocalServiceProvider(this, stub);
      _localServiceProvider.onError = _errorHandler;
    }
  }

  FallbackServiceFactory get fallbackServiceFactory => _fallbackServiceFactory;
  set fallbackServiceFactory(FallbackServiceFactory f) {
    if (_localServiceProvider == null) {
      throw new core.MojoApiError(
          "There must be a local service provider to set a service factory");
    }
    _fallbackServiceFactory = f;
  }

  void requestService(bindings.MojoInterface iface, [String serviceName]) {
    if (iface.ctrl.isBound ||
        (remoteServiceProvider == null) ||
        !remoteServiceProvider.ctrl.isBound) {
      throw new core.MojoApiError(
          "The interface is already bound, "
          "or there is no remote service provider");
    }

    var name = serviceName ?? iface.ctrl.serviceName;
    if ((name == null) || name.isEmpty) {
      throw new core.MojoApiError(
          "If an interface has no ServiceName, then one must be provided.");
    }

    var pipe = new core.MojoMessagePipe();
    iface.ctrl.bind(pipe.endpoints[0]);
    remoteServiceProvider.connectToService_(name, pipe.endpoints[1]);
  }

  /// Prepares this connection to provide the specified service when a call for
  /// the given [interfaceName] is received. The provided service can also
  /// choose to expose a service description.
  void provideService(String interfaceName, ServiceFactory factory,
      {service_describer.ServiceDescription description}) {
    if (_localServiceProvider == null) {
      throw new core.MojoApiError(
          "There must be a local service provider to provide a service");
    }
    if ((interfaceName == null) || interfaceName.isEmpty) {
      throw new core.MojoApiError(
          "The interface name must be non-null and non-empty");
    }

    _nameToServiceFactory[interfaceName] = factory;
    if (description != null) {
      _serviceDescriptions[interfaceName] = description;
      _provideServiceDescriber();
    }
  }

  /// Provides the ServiceDescriber interface for the set of services whose
  /// service descriptions have been provided (see provideService).
  void _provideServiceDescriber() {
    String describerName = service_describer.ServiceDescriber.serviceName;
    if (_nameToServiceFactory[describerName] == null) {
      _nameToServiceFactory[describerName] = (endpoint) =>
          new _ServiceDescriberImpl(_serviceDescriptions, endpoint);
    }
  }

  void _errorHandler(Object e) {
    _localServiceProvider.close().then((_) {
      if (onError != null) onError(e);
      _localServiceProvider = null;
    });
  }

  Future close({bool immediate: false}) {
    var rspCloseFuture;
    var lspCloseFuture;
    if (remoteServiceProvider != null) {
      rspCloseFuture = remoteServiceProvider.close();
      remoteServiceProvider = null;
    } else {
      rspCloseFuture = new Future.value(null);
    }
    if (_localServiceProvider != null) {
      lspCloseFuture = _localServiceProvider.close(immediate: immediate);
      _localServiceProvider = null;
    } else {
      lspCloseFuture = new Future.value(null);
    }
    return rspCloseFuture.then((_) => lspCloseFuture);
  }
}
