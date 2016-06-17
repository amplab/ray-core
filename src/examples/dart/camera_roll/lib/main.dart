// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This example makes use of mojo:camera_roll which is available only when
// running on Android.
//
// Example usage:
//   pub get
//   pub run sky_tools build
//   pub run sky_tools run_mojo --mojo-path=../../.. --android

import 'dart:sky';
import 'dart:typed_data';

import 'package:mojo_services/mojo/camera_roll.mojom.dart';
import 'package:sky/services.dart';

final CameraRollServiceProxy cameraRoll = new CameraRollServiceProxy.unbound();
Image currentImage;
int photoIndex = 0;

Picture paint(Rect paintBounds) {
  PictureRecorder recorder = new PictureRecorder();
  Canvas canvas = new Canvas(recorder, paintBounds);
  Paint paint = new Paint()..color = const Color.fromARGB(255, 0, 255, 0);
  if (currentImage != null) {
    canvas.drawImage(currentImage, new Point(0.0, 0.0), paint);
  }
  return recorder.endRecording();
}

Scene composite(Picture picture, Rect paintBounds) {
  final double devicePixelRatio = view.devicePixelRatio;
  Rect sceneBounds = new Rect.fromLTWH(
      0.0, 0.0, view.width * devicePixelRatio, view.height * devicePixelRatio);
  Float32List deviceTransform = new Float32List(16)
    ..[0] = devicePixelRatio
    ..[5] = devicePixelRatio
    ..[10] = 1.0
    ..[15] = 1.0;
  SceneBuilder sceneBuilder = new SceneBuilder(sceneBounds)
    ..pushTransform(deviceTransform)
    ..addPicture(Offset.zero, picture, paintBounds)
    ..pop();
  return sceneBuilder.build();
}

void beginFrame(double timeStamp) {
  Rect paintBounds = new Rect.fromLTWH(0.0, 0.0, view.width, view.height);
  Picture picture = paint(paintBounds);
  Scene scene = composite(picture, paintBounds);
  view.scene = scene;
}

void getPhoto() {
  var future = cameraRoll.getPhoto(photoIndex);
  future.then((response) {
    if (response.photo == null) {
      print("Photo $photoIndex not found, returning to the first photo.");
      cameraRoll.update();
      photoIndex = 0;
      getPhoto();
      return;
    }

    new ImageDecoder(response.photo.content.handle.h, (image) {
      if (image != null) {
        currentImage = image;
        print("view.scheduleFrame");
        view.scheduleFrame();
      }
    });
  });
}

bool handleEvent(Event event) {
  if (event.type == 'pointerdown') {
    return true;
  }

  if (event.type == 'pointerup') {
    photoIndex++;
    getPhoto();
    return true;
  }

  return false;
}

void main() {
  embedder.connectToService("mojo:camera", cameraRoll);
  view.setFrameCallback(beginFrame);
  view.setEventCallback(handleEvent);
  getPhoto();
}
