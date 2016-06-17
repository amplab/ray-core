// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXAMPLES_MEDIA_TEST_KEYSTROKE_H_
#define EXAMPLES_MEDIA_TEST_KEYSTROKE_H_

namespace mojo {
namespace media {
namespace examples {

// Returns keystroke or 0 if no key has been pressed. This is non-blocking,
// but requires the user to hit enter and doesn't suppress echo. Enter alone
// produces a newline ('\n'). If non-newline characters are entered, the
// terminating newline is suppressed.
char Keystroke();

}  // namespace examples
}  // namespace media
}  // namespace mojo

#endif  // EXAMPLES_MEDIA_TEST_KEYSTROKE_H_
