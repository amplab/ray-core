// WARNING: DO NOT EDIT. This file was generated by a program.
// See $MOJO_SDK/tools/bindings/mojom_bindings_generator.py.

library audio_track_mojom;
import 'dart:async';
import 'package:mojo/bindings.dart' as bindings;
import 'package:mojo/core.dart' as core;
import 'package:mojo/mojo/bindings/types/service_describer.mojom.dart' as service_describer;



class _AudioTrackSetGainParams extends bindings.Struct {
  static const List<bindings.StructDataHeader> kVersions = const [
    const bindings.StructDataHeader(16, 0)
  ];
  double dbGain = 0.0;

  _AudioTrackSetGainParams() : super(kVersions.last.size);

  _AudioTrackSetGainParams.init(
    double this.dbGain
  ) : super(kVersions.last.size);

  static _AudioTrackSetGainParams deserialize(bindings.Message message) =>
      bindings.Struct.deserialize(decode, message);

  static _AudioTrackSetGainParams decode(bindings.Decoder decoder0) {
    if (decoder0 == null) {
      return null;
    }
    _AudioTrackSetGainParams result = new _AudioTrackSetGainParams();

    var mainDataHeader = bindings.Struct.checkVersion(decoder0, kVersions);
    if (mainDataHeader.version >= 0) {
      
      result.dbGain = decoder0.decodeFloat(8);
    }
    return result;
  }

  void encode(bindings.Encoder encoder) {
    var encoder0 = encoder.getStructEncoderAtOffset(kVersions.last);
    const String structName = "_AudioTrackSetGainParams";
    String fieldName;
    try {
      fieldName = "dbGain";
      encoder0.encodeFloat(dbGain, 8);
    } on bindings.MojoCodecError catch(e) {
      bindings.Struct.fixErrorMessage(e, fieldName, structName);
      rethrow;
    }
  }

  String toString() {
    return "_AudioTrackSetGainParams("
           "dbGain: $dbGain" ")";
  }

  Map toJson() {
    Map map = new Map();
    map["dbGain"] = dbGain;
    return map;
  }
}

const int _audioTrackMethodSetGainName = 0;

class _AudioTrackServiceDescription implements service_describer.ServiceDescription {
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

abstract class AudioTrack {
  static const String serviceName = null;

  static service_describer.ServiceDescription _cachedServiceDescription;
  static service_describer.ServiceDescription get serviceDescription {
    if (_cachedServiceDescription == null) {
      _cachedServiceDescription = new _AudioTrackServiceDescription();
    }
    return _cachedServiceDescription;
  }

  static AudioTrackProxy connectToService(
      bindings.ServiceConnector s, String url, [String serviceName]) {
    AudioTrackProxy p = new AudioTrackProxy.unbound();
    String name = serviceName ?? AudioTrack.serviceName;
    if ((name == null) || name.isEmpty) {
      throw new core.MojoApiError(
          "If an interface has no ServiceName, then one must be provided.");
    }
    s.connectToService(url, p, name);
    return p;
  }
  void setGain(double dbGain);
  static const double kMutedGain = -160.0;
  static const double kMaxGain = 20.0;
}

abstract class AudioTrackInterface
    implements bindings.MojoInterface<AudioTrack>,
               AudioTrack {
  factory AudioTrackInterface([AudioTrack impl]) =>
      new AudioTrackStub.unbound(impl);

  factory AudioTrackInterface.fromEndpoint(
      core.MojoMessagePipeEndpoint endpoint,
      [AudioTrack impl]) =>
      new AudioTrackStub.fromEndpoint(endpoint, impl);

  factory AudioTrackInterface.fromMock(
      AudioTrack mock) =>
      new AudioTrackProxy.fromMock(mock);
}

abstract class AudioTrackInterfaceRequest
    implements bindings.MojoInterface<AudioTrack>,
               AudioTrack {
  factory AudioTrackInterfaceRequest() =>
      new AudioTrackProxy.unbound();
}

class _AudioTrackProxyControl
    extends bindings.ProxyMessageHandler
    implements bindings.ProxyControl<AudioTrack> {
  AudioTrack impl;

  _AudioTrackProxyControl.fromEndpoint(
      core.MojoMessagePipeEndpoint endpoint) : super.fromEndpoint(endpoint);

  _AudioTrackProxyControl.fromHandle(
      core.MojoHandle handle) : super.fromHandle(handle);

  _AudioTrackProxyControl.unbound() : super.unbound();

  String get serviceName => AudioTrack.serviceName;

  void handleResponse(bindings.ServiceMessage message) {
    switch (message.header.type) {
      default:
        proxyError("Unexpected message type: ${message.header.type}");
        close(immediate: true);
        break;
    }
  }

  @override
  String toString() {
    var superString = super.toString();
    return "_AudioTrackProxyControl($superString)";
  }
}

class AudioTrackProxy
    extends bindings.Proxy<AudioTrack>
    implements AudioTrack,
               AudioTrackInterface,
               AudioTrackInterfaceRequest {
  AudioTrackProxy.fromEndpoint(
      core.MojoMessagePipeEndpoint endpoint)
      : super(new _AudioTrackProxyControl.fromEndpoint(endpoint));

  AudioTrackProxy.fromHandle(core.MojoHandle handle)
      : super(new _AudioTrackProxyControl.fromHandle(handle));

  AudioTrackProxy.unbound()
      : super(new _AudioTrackProxyControl.unbound());

  factory AudioTrackProxy.fromMock(AudioTrack mock) {
    AudioTrackProxy newMockedProxy =
        new AudioTrackProxy.unbound();
    newMockedProxy.impl = mock;
    return newMockedProxy;
  }

  static AudioTrackProxy newFromEndpoint(
      core.MojoMessagePipeEndpoint endpoint) {
    assert(endpoint.setDescription("For AudioTrackProxy"));
    return new AudioTrackProxy.fromEndpoint(endpoint);
  }


  void setGain(double dbGain) {
    if (impl != null) {
      impl.setGain(dbGain);
      return;
    }
    if (!ctrl.isBound) {
      ctrl.proxyError("The Proxy is closed.");
      return;
    }
    var params = new _AudioTrackSetGainParams();
    params.dbGain = dbGain;
    ctrl.sendMessage(params,
        _audioTrackMethodSetGainName);
  }
}

class _AudioTrackStubControl
    extends bindings.StubMessageHandler
    implements bindings.StubControl<AudioTrack> {
  AudioTrack _impl;

  _AudioTrackStubControl.fromEndpoint(
      core.MojoMessagePipeEndpoint endpoint, [AudioTrack impl])
      : super.fromEndpoint(endpoint, autoBegin: impl != null) {
    _impl = impl;
  }

  _AudioTrackStubControl.fromHandle(
      core.MojoHandle handle, [AudioTrack impl])
      : super.fromHandle(handle, autoBegin: impl != null) {
    _impl = impl;
  }

  _AudioTrackStubControl.unbound([this._impl]) : super.unbound();

  String get serviceName => AudioTrack.serviceName;



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
      case _audioTrackMethodSetGainName:
        var params = _AudioTrackSetGainParams.deserialize(
            message.payload);
        _impl.setGain(params.dbGain);
        break;
      default:
        throw new bindings.MojoCodecError("Unexpected message name");
        break;
    }
  }

  AudioTrack get impl => _impl;
  set impl(AudioTrack d) {
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
    return "_AudioTrackStubControl($superString)";
  }

  int get version => 0;
}

class AudioTrackStub
    extends bindings.Stub<AudioTrack>
    implements AudioTrack,
               AudioTrackInterface,
               AudioTrackInterfaceRequest {
  AudioTrackStub.unbound([AudioTrack impl])
      : super(new _AudioTrackStubControl.unbound(impl));

  AudioTrackStub.fromEndpoint(
      core.MojoMessagePipeEndpoint endpoint, [AudioTrack impl])
      : super(new _AudioTrackStubControl.fromEndpoint(endpoint, impl));

  AudioTrackStub.fromHandle(
      core.MojoHandle handle, [AudioTrack impl])
      : super(new _AudioTrackStubControl.fromHandle(handle, impl));

  static AudioTrackStub newFromEndpoint(
      core.MojoMessagePipeEndpoint endpoint) {
    assert(endpoint.setDescription("For AudioTrackStub"));
    return new AudioTrackStub.fromEndpoint(endpoint);
  }


  void setGain(double dbGain) {
    return impl.setGain(dbGain);
  }
}



