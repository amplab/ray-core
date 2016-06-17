// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This example demonstrates how you can serialize and deserialize a generated
// mojom into and out of a buffer.

#include "examples/serialization/serialization.mojom.h"
#include "mojo/public/cpp/bindings/array.h"
#include "mojo/public/cpp/environment/logging.h"

int main() {
  examples::MyStruct in;
  examples::MyStruct out;

  in.a = 1;
  in.b = 2.0f;
  in.c = "hello world!";

  char buf[1000];
  MOJO_CHECK(in.Serialize(buf, sizeof(buf)));
  MOJO_CHECK(out.Deserialize(buf, sizeof(buf)));
  MOJO_CHECK(out.a == 1);
  MOJO_CHECK(out.b == 2.0f);
  MOJO_CHECK(out.c == "hello world!");

  return 0;
}
