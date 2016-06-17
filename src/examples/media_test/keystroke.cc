// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <iostream>

#include "examples/media_test/keystroke.h"

namespace mojo {
namespace media {
namespace examples {

namespace {

bool eat_newline = false;
bool upped_already = false;

}  // namespace

char Keystroke(void) {
  const char* kUp = "\033[A";

  fcntl(STDIN_FILENO, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
  char buf[1];
  if (read(STDIN_FILENO, buf, 1) > 0) {
    if (!upped_already) {
      std::cout << kUp << std::flush;
      upped_already = true;
    }

    if (buf[0] == '\n') {
      upped_already = false;
      if (eat_newline) {
        eat_newline = false;
      } else {
        return buf[0];
      }
    } else {
      eat_newline = true;
      return buf[0];
    }
  }
  return 0;
}

}  // namespace examples
}  // namespace media
}  // namespace mojo
