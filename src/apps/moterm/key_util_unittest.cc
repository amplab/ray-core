// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/moterm/key_util.h"

#include <string>

#include "mojo/services/input_events/interfaces/input_events.mojom.h"
#include "mojo/services/input_events/interfaces/input_key_codes.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// Makes a key event. Note that this cheats in a number of respects.
// TODO(vtl): Add a |flags| argument when |GetInputSequenceForKeyPressedEvent()|
// actually uses it.
mojo::EventPtr MakeKeyEvent(bool is_char,
                            char character,
                            mojo::KeyboardCode windows_key_code) {
  uint16_t character16 = static_cast<unsigned char>(character);

  mojo::EventPtr ev = mojo::Event::New();
  ev->action = mojo::EventType::KEY_PRESSED;
  ev->flags = mojo::EventFlags::NONE;  // Cheat.
  ev->time_stamp = 1234567890LL;
  ev->key_data = mojo::KeyData::New();
  ev->key_data->key_code = 0;  // Cheat.
  ev->key_data->is_char = is_char;
  ev->key_data->character = character16;
  ev->key_data->windows_key_code = windows_key_code;
  ev->key_data->native_key_code = 0;
  ev->key_data->text = character16;             // Cheat.
  ev->key_data->unmodified_text = character16;  // Cheat.
  return ev;
}

TEST(KeyUtilTest, RegularChars) {
  // Only handles character events.
  EXPECT_EQ(std::string(),
            GetInputSequenceForKeyPressedEvent(
                *MakeKeyEvent(false, 0, mojo::KeyboardCode::A), false));

  EXPECT_EQ("A", GetInputSequenceForKeyPressedEvent(
                     *MakeKeyEvent(true, 'A', mojo::KeyboardCode::A), false));
  EXPECT_EQ("a", GetInputSequenceForKeyPressedEvent(
                     *MakeKeyEvent(true, 'a', mojo::KeyboardCode::A), false));
  EXPECT_EQ("0",
            GetInputSequenceForKeyPressedEvent(
                *MakeKeyEvent(true, '0', mojo::KeyboardCode::NUM_0), false));
  EXPECT_EQ("!",
            GetInputSequenceForKeyPressedEvent(
                *MakeKeyEvent(true, '!', mojo::KeyboardCode::NUM_0), false));
  EXPECT_EQ("\t",
            GetInputSequenceForKeyPressedEvent(
                *MakeKeyEvent(true, '\t', mojo::KeyboardCode::TAB), false));
  EXPECT_EQ("\r",
            GetInputSequenceForKeyPressedEvent(
                *MakeKeyEvent(true, '\r', mojo::KeyboardCode::RETURN), false));
}

TEST(KeyUtilTest, SpecialChars) {
  // Backspace should send DEL.
  EXPECT_EQ("\x7f",
            GetInputSequenceForKeyPressedEvent(
                *MakeKeyEvent(true, 0, mojo::KeyboardCode::BACK), false));
  EXPECT_EQ("\x1b",
            GetInputSequenceForKeyPressedEvent(
                *MakeKeyEvent(true, 0, mojo::KeyboardCode::ESCAPE), false));
  // Some multi-character results:
  EXPECT_EQ("\x1b[5~",
            GetInputSequenceForKeyPressedEvent(
                *MakeKeyEvent(true, 0, mojo::KeyboardCode::PRIOR), false));
  EXPECT_EQ("\x1b[H",
            GetInputSequenceForKeyPressedEvent(
                *MakeKeyEvent(true, 0, mojo::KeyboardCode::HOME), false));
  EXPECT_EQ("\x1b[C",
            GetInputSequenceForKeyPressedEvent(
                *MakeKeyEvent(true, 0, mojo::KeyboardCode::RIGHT), false));
}

TEST(KeyUtilTest, KeypadApplicationMode) {
  // Page-up should not be affected by keypad application mode.
  EXPECT_EQ("\x1b[5~",
            GetInputSequenceForKeyPressedEvent(
                *MakeKeyEvent(true, 0, mojo::KeyboardCode::PRIOR), true));
  // But Home (and End) should be.
  EXPECT_EQ("\x1bOH",
            GetInputSequenceForKeyPressedEvent(
                *MakeKeyEvent(true, 0, mojo::KeyboardCode::HOME), true));
  // As should the arroy keys.
  EXPECT_EQ("\x1bOC",
            GetInputSequenceForKeyPressedEvent(
                *MakeKeyEvent(true, 0, mojo::KeyboardCode::RIGHT), true));
  // TODO(vtl): Test this more thoroughly. Also other keypad keys (once that's
  // implemented).
}

}  // namespace
