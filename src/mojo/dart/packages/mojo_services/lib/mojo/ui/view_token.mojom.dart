// WARNING: DO NOT EDIT. This file was generated by a program.
// See $MOJO_SDK/tools/bindings/mojom_bindings_generator.py.

library view_token_mojom;
import 'dart:async';
import 'package:mojo/bindings.dart' as bindings;
import 'package:mojo/core.dart' as core;
import 'package:mojo/mojo/bindings/types/service_describer.mojom.dart' as service_describer;



class ViewToken extends bindings.Struct {
  static const List<bindings.StructDataHeader> kVersions = const [
    const bindings.StructDataHeader(16, 0)
  ];
  int value = 0;

  ViewToken() : super(kVersions.last.size);

  ViewToken.init(
    int this.value
  ) : super(kVersions.last.size);

  static ViewToken deserialize(bindings.Message message) =>
      bindings.Struct.deserialize(decode, message);

  static ViewToken decode(bindings.Decoder decoder0) {
    if (decoder0 == null) {
      return null;
    }
    ViewToken result = new ViewToken();

    var mainDataHeader = bindings.Struct.checkVersion(decoder0, kVersions);
    if (mainDataHeader.version >= 0) {
      
      result.value = decoder0.decodeUint32(8);
    }
    return result;
  }

  void encode(bindings.Encoder encoder) {
    var encoder0 = encoder.getStructEncoderAtOffset(kVersions.last);
    const String structName = "ViewToken";
    String fieldName;
    try {
      fieldName = "value";
      encoder0.encodeUint32(value, 8);
    } on bindings.MojoCodecError catch(e) {
      bindings.Struct.fixErrorMessage(e, fieldName, structName);
      rethrow;
    }
  }

  String toString() {
    return "ViewToken("
           "value: $value" ")";
  }

  Map toJson() {
    Map map = new Map();
    map["value"] = value;
    return map;
  }
}


class _ViewOwnerGetTokenParams extends bindings.Struct {
  static const List<bindings.StructDataHeader> kVersions = const [
    const bindings.StructDataHeader(8, 0)
  ];

  _ViewOwnerGetTokenParams() : super(kVersions.last.size);

  _ViewOwnerGetTokenParams.init(
  ) : super(kVersions.last.size);

  static _ViewOwnerGetTokenParams deserialize(bindings.Message message) =>
      bindings.Struct.deserialize(decode, message);

  static _ViewOwnerGetTokenParams decode(bindings.Decoder decoder0) {
    if (decoder0 == null) {
      return null;
    }
    _ViewOwnerGetTokenParams result = new _ViewOwnerGetTokenParams();

    var mainDataHeader = bindings.Struct.checkVersion(decoder0, kVersions);
    return result;
  }

  void encode(bindings.Encoder encoder) {
    encoder.getStructEncoderAtOffset(kVersions.last);
    const String structName = "_ViewOwnerGetTokenParams";
    String fieldName;
    try {
    } on bindings.MojoCodecError catch(e) {
      bindings.Struct.fixErrorMessage(e, fieldName, structName);
      rethrow;
    }
  }

  String toString() {
    return "_ViewOwnerGetTokenParams("")";
  }

  Map toJson() {
    Map map = new Map();
    return map;
  }
}


class ViewOwnerGetTokenResponseParams extends bindings.Struct {
  static const List<bindings.StructDataHeader> kVersions = const [
    const bindings.StructDataHeader(16, 0)
  ];
  ViewToken token = null;

  ViewOwnerGetTokenResponseParams() : super(kVersions.last.size);

  ViewOwnerGetTokenResponseParams.init(
    ViewToken this.token
  ) : super(kVersions.last.size);

  static ViewOwnerGetTokenResponseParams deserialize(bindings.Message message) =>
      bindings.Struct.deserialize(decode, message);

  static ViewOwnerGetTokenResponseParams decode(bindings.Decoder decoder0) {
    if (decoder0 == null) {
      return null;
    }
    ViewOwnerGetTokenResponseParams result = new ViewOwnerGetTokenResponseParams();

    var mainDataHeader = bindings.Struct.checkVersion(decoder0, kVersions);
    if (mainDataHeader.version >= 0) {
      
      var decoder1 = decoder0.decodePointer(8, false);
      result.token = ViewToken.decode(decoder1);
    }
    return result;
  }

  void encode(bindings.Encoder encoder) {
    var encoder0 = encoder.getStructEncoderAtOffset(kVersions.last);
    const String structName = "ViewOwnerGetTokenResponseParams";
    String fieldName;
    try {
      fieldName = "token";
      encoder0.encodeStruct(token, 8, false);
    } on bindings.MojoCodecError catch(e) {
      bindings.Struct.fixErrorMessage(e, fieldName, structName);
      rethrow;
    }
  }

  String toString() {
    return "ViewOwnerGetTokenResponseParams("
           "token: $token" ")";
  }

  Map toJson() {
    Map map = new Map();
    map["token"] = token;
    return map;
  }
}

const int _viewOwnerMethodGetTokenName = 0;

class _ViewOwnerServiceDescription implements service_describer.ServiceDescription {
  void getTopLevelInterface(Function responder) {
    responder(null);
  }

  void getTypeDefinition(String typeKey, Function responder) {
    responder(null);
  }

  void getAllTypeDefinitions(Function responder) {
    responder(null);
  }
}

abstract class ViewOwner {
  static const String serviceName = null;

  static service_describer.ServiceDescription _cachedServiceDescription;
  static service_describer.ServiceDescription get serviceDescription {
    if (_cachedServiceDescription == null) {
      _cachedServiceDescription = new _ViewOwnerServiceDescription();
    }
    return _cachedServiceDescription;
  }

  static ViewOwnerProxy connectToService(
      bindings.ServiceConnector s, String url, [String serviceName]) {
    ViewOwnerProxy p = new ViewOwnerProxy.unbound();
    String name = serviceName ?? ViewOwner.serviceName;
    if ((name == null) || name.isEmpty) {
      throw new core.MojoApiError(
          "If an interface has no ServiceName, then one must be provided.");
    }
    s.connectToService(url, p, name);
    return p;
  }
  void getToken(void callback(ViewToken token));
}

abstract class ViewOwnerInterface
    implements bindings.MojoInterface<ViewOwner>,
               ViewOwner {
  factory ViewOwnerInterface([ViewOwner impl]) =>
      new ViewOwnerStub.unbound(impl);

  factory ViewOwnerInterface.fromEndpoint(
      core.MojoMessagePipeEndpoint endpoint,
      [ViewOwner impl]) =>
      new ViewOwnerStub.fromEndpoint(endpoint, impl);

  factory ViewOwnerInterface.fromMock(
      ViewOwner mock) =>
      new ViewOwnerProxy.fromMock(mock);
}

abstract class ViewOwnerInterfaceRequest
    implements bindings.MojoInterface<ViewOwner>,
               ViewOwner {
  factory ViewOwnerInterfaceRequest() =>
      new ViewOwnerProxy.unbound();
}

class _ViewOwnerProxyControl
    extends bindings.ProxyMessageHandler
    implements bindings.ProxyControl<ViewOwner> {
  ViewOwner impl;

  _ViewOwnerProxyControl.fromEndpoint(
      core.MojoMessagePipeEndpoint endpoint) : super.fromEndpoint(endpoint);

  _ViewOwnerProxyControl.fromHandle(
      core.MojoHandle handle) : super.fromHandle(handle);

  _ViewOwnerProxyControl.unbound() : super.unbound();

  String get serviceName => ViewOwner.serviceName;

  void handleResponse(bindings.ServiceMessage message) {
    switch (message.header.type) {
      case _viewOwnerMethodGetTokenName:
        var r = ViewOwnerGetTokenResponseParams.deserialize(
            message.payload);
        if (!message.header.hasRequestId) {
          proxyError("Expected a message with a valid request Id.");
          return;
        }
        Function callback = callbackMap[message.header.requestId];
        if (callback == null) {
          proxyError(
              "Message had unknown request Id: ${message.header.requestId}");
          return;
        }
        callbackMap.remove(message.header.requestId);
        callback(r.token );
        break;
      default:
        proxyError("Unexpected message type: ${message.header.type}");
        close(immediate: true);
        break;
    }
  }

  @override
  String toString() {
    var superString = super.toString();
    return "_ViewOwnerProxyControl($superString)";
  }
}

class ViewOwnerProxy
    extends bindings.Proxy<ViewOwner>
    implements ViewOwner,
               ViewOwnerInterface,
               ViewOwnerInterfaceRequest {
  ViewOwnerProxy.fromEndpoint(
      core.MojoMessagePipeEndpoint endpoint)
      : super(new _ViewOwnerProxyControl.fromEndpoint(endpoint));

  ViewOwnerProxy.fromHandle(core.MojoHandle handle)
      : super(new _ViewOwnerProxyControl.fromHandle(handle));

  ViewOwnerProxy.unbound()
      : super(new _ViewOwnerProxyControl.unbound());

  factory ViewOwnerProxy.fromMock(ViewOwner mock) {
    ViewOwnerProxy newMockedProxy =
        new ViewOwnerProxy.unbound();
    newMockedProxy.impl = mock;
    return newMockedProxy;
  }

  static ViewOwnerProxy newFromEndpoint(
      core.MojoMessagePipeEndpoint endpoint) {
    assert(endpoint.setDescription("For ViewOwnerProxy"));
    return new ViewOwnerProxy.fromEndpoint(endpoint);
  }


  void getToken(void callback(ViewToken token)) {
    if (impl != null) {
      impl.getToken(callback);
      return;
    }
    var params = new _ViewOwnerGetTokenParams();
    Function zonedCallback;
    if (identical(Zone.current, Zone.ROOT)) {
      zonedCallback = callback;
    } else {
      Zone z = Zone.current;
      zonedCallback = ((ViewToken token) {
        z.bindCallback(() {
          callback(token);
        })();
      });
    }
    ctrl.sendMessageWithRequestId(
        params,
        _viewOwnerMethodGetTokenName,
        -1,
        bindings.MessageHeader.kMessageExpectsResponse,
        zonedCallback);
  }
}

class _ViewOwnerStubControl
    extends bindings.StubMessageHandler
    implements bindings.StubControl<ViewOwner> {
  ViewOwner _impl;

  _ViewOwnerStubControl.fromEndpoint(
      core.MojoMessagePipeEndpoint endpoint, [ViewOwner impl])
      : super.fromEndpoint(endpoint, autoBegin: impl != null) {
    _impl = impl;
  }

  _ViewOwnerStubControl.fromHandle(
      core.MojoHandle handle, [ViewOwner impl])
      : super.fromHandle(handle, autoBegin: impl != null) {
    _impl = impl;
  }

  _ViewOwnerStubControl.unbound([this._impl]) : super.unbound();

  String get serviceName => ViewOwner.serviceName;


  Function _viewOwnerGetTokenResponseParamsResponder(
      int requestId) {
  return (ViewToken token) {
      var result = new ViewOwnerGetTokenResponseParams();
      result.token = token;
      sendResponse(buildResponseWithId(
          result,
          _viewOwnerMethodGetTokenName,
          requestId,
          bindings.MessageHeader.kMessageIsResponse));
    };
  }

  void handleMessage(bindings.ServiceMessage message) {
    if (bindings.ControlMessageHandler.isControlMessage(message)) {
      bindings.ControlMessageHandler.handleMessage(
          this, 0, message);
      return;
    }
    if (_impl == null) {
      throw new core.MojoApiError("$this has no implementation set");
    }
    switch (message.header.type) {
      case _viewOwnerMethodGetTokenName:
        _impl.getToken(_viewOwnerGetTokenResponseParamsResponder(message.header.requestId));
        break;
      default:
        throw new bindings.MojoCodecError("Unexpected message name");
        break;
    }
  }

  ViewOwner get impl => _impl;
  set impl(ViewOwner d) {
    if (d == null) {
      throw new core.MojoApiError("$this: Cannot set a null implementation");
    }
    if (isBound && (_impl == null)) {
      beginHandlingEvents();
    }
    _impl = d;
  }

  @override
  void bind(core.MojoMessagePipeEndpoint endpoint) {
    super.bind(endpoint);
    if (!isOpen && (_impl != null)) {
      beginHandlingEvents();
    }
  }

  @override
  String toString() {
    var superString = super.toString();
    return "_ViewOwnerStubControl($superString)";
  }

  int get version => 0;
}

class ViewOwnerStub
    extends bindings.Stub<ViewOwner>
    implements ViewOwner,
               ViewOwnerInterface,
               ViewOwnerInterfaceRequest {
  ViewOwnerStub.unbound([ViewOwner impl])
      : super(new _ViewOwnerStubControl.unbound(impl));

  ViewOwnerStub.fromEndpoint(
      core.MojoMessagePipeEndpoint endpoint, [ViewOwner impl])
      : super(new _ViewOwnerStubControl.fromEndpoint(endpoint, impl));

  ViewOwnerStub.fromHandle(
      core.MojoHandle handle, [ViewOwner impl])
      : super(new _ViewOwnerStubControl.fromHandle(handle, impl));

  static ViewOwnerStub newFromEndpoint(
      core.MojoMessagePipeEndpoint endpoint) {
    assert(endpoint.setDescription("For ViewOwnerStub"));
    return new ViewOwnerStub.fromEndpoint(endpoint);
  }


  void getToken(void callback(ViewToken token)) {
    return impl.getToken(callback);
  }
}



