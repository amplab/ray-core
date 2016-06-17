// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This example makes use of mojo:camera which is available only when
// running on Android. It repeatedly captures camera video frame images
// and displays it in a mojo view.
//
// Example usage:
//   pub get
//   pub run flutter build
//   pub run flutter run_mojo --mojo-path=../../.. --android

import 'dart:math' as math;
import 'dart:typed_data';
import 'dart:ui' as ui;

import 'package:flutter/painting.dart';
import 'package:flutter/services.dart';
import 'package:flutter/widgets.dart';
import 'package:mojo_services/mojo/camera.mojom.dart';

ui.Image image = null;
final CameraServiceProxy camera = new CameraServiceProxy.unbound();

ui.Picture paint(ui.Rect paintBounds) {
  ui.PictureRecorder recorder = new ui.PictureRecorder();
  if (image != null) {
    ui.Canvas canvas = new ui.Canvas(recorder, paintBounds);
    canvas.translate(paintBounds.width / 2.0, paintBounds.height / 2.0);
    ui.Paint paint = new Paint()..color = const Color.fromARGB(255, 0, 255, 0);
    // TODO(alhaad): Use orientation information from Flutter.
    if (ui.view.width < ui.view.height)
      canvas.rotate(math.PI / 2);
    canvas.drawImage(
        image, new Point(-image.width / 2.0, -image.height / 2.0), paint);
  }
  return recorder.endRecording();
}

ui.Scene composite(ui.Picture picture, ui.Rect paintBounds) {
  final double devicePixelRatio = ui.view.devicePixelRatio;
  ui.Rect sceneBounds = new ui.Rect.fromLTWH(0.0, 0.0,
      ui.view.width * devicePixelRatio, ui.view.height * devicePixelRatio);
  Float64List deviceTransform = new Float64List(16)
    ..[0] = devicePixelRatio
    ..[5] = devicePixelRatio
    ..[10] = 1.0
    ..[15] = 1.0;
  ui.SceneBuilder sceneBuilder = new ui.SceneBuilder(sceneBounds)
    ..pushTransform(deviceTransform)
    ..addPicture(ui.Offset.zero, picture, paintBounds)
    ..pop();
  return sceneBuilder.build();
}

void beginFrame(double timeStamp) {
  Rect paintBounds = new Rect.fromLTWH(0.0, 0.0, ui.view.width, ui.view.height);
  Picture picture = paint(paintBounds);
  ui.Scene scene = composite(picture, paintBounds);
  ui.view.scene = scene;
}

void drawNextPhoto() {
  var future = camera.getLatestFrame();
  future.then((response) {
    if (response.content == null) {
      drawNextPhoto();
      return;
    }

    final imageFrame = decodeImageFromDataPipe(response.content);
    imageFrame.then((frame) {
      if (frame != null) {
        image = frame;
        ui.view.scheduleFrame();
        drawNextPhoto();
      }
    });
  });
}

void main() {
  embedder.connectToService("mojo:camera", camera);
  drawNextPhoto();
  ui.view.setFrameCallback(beginFrame);
}
