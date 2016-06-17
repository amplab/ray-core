// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/moterm/key_util.h"

#include "base/logging.h"
#include "mojo/services/input_events/interfaces/input_events.mojom.h"
#include "mojo/services/input_events/interfaces/input_key_codes.mojom.h"

// TODO(vtl): Handle more stuff and verify that we're consistent about the
// sequences we generate.
// TODO(vtl): In particular, our implementation of keypad_application_mode is
// incomplete.
std::string GetInputSequenceForKeyPressedEvent(const mojo::Event& key_event,
                                               bool keypad_application_mode) {
  DCHECK_EQ(key_event.action, mojo::EventType::KEY_PRESSED);
  CHECK(key_event.key_data);
  const mojo::KeyData& key_data = *key_event.key_data;

  DVLOG(2) << "Key pressed:"
           << "\n  is_char = " << key_data.is_char
           << "\n  character = " << key_data.character
           << "\n  windows_key_code = " << key_data.windows_key_code
           << "\n  text = " << key_data.text
           << "\n  unmodified_text = " << key_data.unmodified_text;

  // We'll only deal with character events (which we'll get even if |character|
  // isn't set).
  if (!key_data.is_char)
    return std::string();

  // Use |character| if that's set.
  // TODO(vtl): Maybe we should use |text| instead, but it seems to be the same
  // as |character|. (The comments claim that |text| will have something for
  // backspace while |character| won't, but this does not appear to be true
  // currently.)
  if (key_data.character) {
    if (key_data.character >= 128) {
      // TODO(vtl): Need to UTF-8 encode.
      NOTIMPLEMENTED();
      return std::string();
    }
    return std::string(1, static_cast<char>(key_data.character));
  }

  // TODO(vtl): For some of these, we may need to handle modifiers (from
  // |event.flags|).
  switch (key_data.windows_key_code) {
    // Produces input sequence:
    case mojo::KeyboardCode::BACK:
      // Have backspace send DEL instead of BS.
      return std::string("\x7f");
    case mojo::KeyboardCode::ESCAPE:
      return std::string("\x1b");
    case mojo::KeyboardCode::PRIOR:
      return std::string("\x1b[5~");
    case mojo::KeyboardCode::NEXT:
      return std::string("\x1b[6~");
    case mojo::KeyboardCode::END:
      return std::string(keypad_application_mode ? "\x1bOF" : "\x1b[F");
    case mojo::KeyboardCode::HOME:
      return std::string(keypad_application_mode ? "\x1bOH" : "\x1b[H");
    case mojo::KeyboardCode::LEFT:
      return std::string(keypad_application_mode ? "\x1bOD" : "\x1b[D");
    case mojo::KeyboardCode::UP:
      return std::string(keypad_application_mode ? "\x1bOA" : "\x1b[A");
    case mojo::KeyboardCode::RIGHT:
      return std::string(keypad_application_mode ? "\x1bOC" : "\x1b[C");
    case mojo::KeyboardCode::DOWN:
      return std::string(keypad_application_mode ? "\x1bOB" : "\x1b[B");
    case mojo::KeyboardCode::INSERT:
      return std::string("\x1b[2~");
    case mojo::KeyboardCode::DELETE:
      return std::string("\x1b[3~");

    // Should have |character| set:
    case mojo::KeyboardCode::TAB:
    case mojo::KeyboardCode::RETURN:
    case mojo::KeyboardCode::SPACE:
    case mojo::KeyboardCode::NUM_0:
    case mojo::KeyboardCode::NUM_1:
    case mojo::KeyboardCode::NUM_2:
    case mojo::KeyboardCode::NUM_3:
    case mojo::KeyboardCode::NUM_4:
    case mojo::KeyboardCode::NUM_5:
    case mojo::KeyboardCode::NUM_6:
    case mojo::KeyboardCode::NUM_7:
    case mojo::KeyboardCode::NUM_8:
    case mojo::KeyboardCode::NUM_9:
    case mojo::KeyboardCode::A:
    case mojo::KeyboardCode::B:
    case mojo::KeyboardCode::C:
    case mojo::KeyboardCode::D:
    case mojo::KeyboardCode::E:
    case mojo::KeyboardCode::F:
    case mojo::KeyboardCode::G:
    case mojo::KeyboardCode::H:
    case mojo::KeyboardCode::I:
    case mojo::KeyboardCode::J:
    case mojo::KeyboardCode::K:
    case mojo::KeyboardCode::L:
    case mojo::KeyboardCode::M:
    case mojo::KeyboardCode::N:
    case mojo::KeyboardCode::O:
    case mojo::KeyboardCode::P:
    case mojo::KeyboardCode::Q:
    case mojo::KeyboardCode::R:
    case mojo::KeyboardCode::S:
    case mojo::KeyboardCode::T:
    case mojo::KeyboardCode::U:
    case mojo::KeyboardCode::V:
    case mojo::KeyboardCode::W:
    case mojo::KeyboardCode::X:
    case mojo::KeyboardCode::Y:
    case mojo::KeyboardCode::Z:
      // TODO(vtl): Actually, we won't get characters for Ctrl+<number> (and
      // probably other odd combinations).
      DLOG(WARNING) << "Expected character for key code "
                    << key_data.windows_key_code;
      break;

    // Purposely produce no input sequence:
    case mojo::KeyboardCode::SHIFT:
    case mojo::KeyboardCode::CONTROL:
    case mojo::KeyboardCode::MENU:
    case mojo::KeyboardCode::LSHIFT:
    case mojo::KeyboardCode::RSHIFT:
    case mojo::KeyboardCode::LCONTROL:
    case mojo::KeyboardCode::RCONTROL:
    case mojo::KeyboardCode::LMENU:
    case mojo::KeyboardCode::RMENU:
      break;

    // TODO(vtl): Figure these out.
    case mojo::KeyboardCode::CLEAR:
    case mojo::KeyboardCode::PAUSE:
    case mojo::KeyboardCode::CAPITAL:
    case mojo::KeyboardCode::KANA:  // A.k.a. |KeyboardCode::HANGUL|.
    case mojo::KeyboardCode::JUNJA:
    case mojo::KeyboardCode::FINAL:
    case mojo::KeyboardCode::HANJA:  // A.k.a. |KeyboardCode::KANJI|.
    case mojo::KeyboardCode::CONVERT:
    case mojo::KeyboardCode::NONCONVERT:
    case mojo::KeyboardCode::ACCEPT:
    case mojo::KeyboardCode::MODECHANGE:
    case mojo::KeyboardCode::SELECT:
    case mojo::KeyboardCode::PRINT:
    case mojo::KeyboardCode::EXECUTE:
    case mojo::KeyboardCode::SNAPSHOT:
    case mojo::KeyboardCode::HELP:
    case mojo::KeyboardCode::LWIN:  // A.k.a. |KeyboardCode::COMMAND|.
    case mojo::KeyboardCode::RWIN:
    case mojo::KeyboardCode::APPS:
    case mojo::KeyboardCode::SLEEP:
    case mojo::KeyboardCode::NUMPAD0:
    case mojo::KeyboardCode::NUMPAD1:
    case mojo::KeyboardCode::NUMPAD2:
    case mojo::KeyboardCode::NUMPAD3:
    case mojo::KeyboardCode::NUMPAD4:
    case mojo::KeyboardCode::NUMPAD5:
    case mojo::KeyboardCode::NUMPAD6:
    case mojo::KeyboardCode::NUMPAD7:
    case mojo::KeyboardCode::NUMPAD8:
    case mojo::KeyboardCode::NUMPAD9:
    case mojo::KeyboardCode::MULTIPLY:
    case mojo::KeyboardCode::ADD:
    case mojo::KeyboardCode::SEPARATOR:
    case mojo::KeyboardCode::SUBTRACT:
    case mojo::KeyboardCode::DECIMAL:
    case mojo::KeyboardCode::DIVIDE:
    case mojo::KeyboardCode::F1:
    case mojo::KeyboardCode::F2:
    case mojo::KeyboardCode::F3:
    case mojo::KeyboardCode::F4:
    case mojo::KeyboardCode::F5:
    case mojo::KeyboardCode::F6:
    case mojo::KeyboardCode::F7:
    case mojo::KeyboardCode::F8:
    case mojo::KeyboardCode::F9:
    case mojo::KeyboardCode::F10:
    case mojo::KeyboardCode::F11:
    case mojo::KeyboardCode::F12:
    case mojo::KeyboardCode::F13:
    case mojo::KeyboardCode::F14:
    case mojo::KeyboardCode::F15:
    case mojo::KeyboardCode::F16:
    case mojo::KeyboardCode::F17:
    case mojo::KeyboardCode::F18:
    case mojo::KeyboardCode::F19:
    case mojo::KeyboardCode::F20:
    case mojo::KeyboardCode::F21:
    case mojo::KeyboardCode::F22:
    case mojo::KeyboardCode::F23:
    case mojo::KeyboardCode::F24:
    case mojo::KeyboardCode::NUMLOCK:
    case mojo::KeyboardCode::SCROLL:
    case mojo::KeyboardCode::BROWSER_BACK:
    case mojo::KeyboardCode::BROWSER_FORWARD:
    case mojo::KeyboardCode::BROWSER_REFRESH:
    case mojo::KeyboardCode::BROWSER_STOP:
    case mojo::KeyboardCode::BROWSER_SEARCH:
    case mojo::KeyboardCode::BROWSER_FAVORITES:
    case mojo::KeyboardCode::BROWSER_HOME:
    case mojo::KeyboardCode::VOLUME_MUTE:
    case mojo::KeyboardCode::VOLUME_DOWN:
    case mojo::KeyboardCode::VOLUME_UP:
    case mojo::KeyboardCode::MEDIA_NEXT_TRACK:
    case mojo::KeyboardCode::MEDIA_PREV_TRACK:
    case mojo::KeyboardCode::MEDIA_STOP:
    case mojo::KeyboardCode::MEDIA_PLAY_PAUSE:
    case mojo::KeyboardCode::MEDIA_LAUNCH_MAIL:
    case mojo::KeyboardCode::MEDIA_LAUNCH_MEDIA_SELECT:
    case mojo::KeyboardCode::MEDIA_LAUNCH_APP1:
    case mojo::KeyboardCode::MEDIA_LAUNCH_APP2:
    case mojo::KeyboardCode::OEM_1:
    case mojo::KeyboardCode::OEM_PLUS:
    case mojo::KeyboardCode::OEM_COMMA:
    case mojo::KeyboardCode::OEM_MINUS:
    case mojo::KeyboardCode::OEM_PERIOD:
    case mojo::KeyboardCode::OEM_2:
    case mojo::KeyboardCode::OEM_3:
    case mojo::KeyboardCode::OEM_4:
    case mojo::KeyboardCode::OEM_5:
    case mojo::KeyboardCode::OEM_6:
    case mojo::KeyboardCode::OEM_7:
    case mojo::KeyboardCode::OEM_8:
    case mojo::KeyboardCode::OEM_102:
    case mojo::KeyboardCode::PROCESSKEY:
    case mojo::KeyboardCode::PACKET:
    case mojo::KeyboardCode::DBE_SBCSCHAR:
    case mojo::KeyboardCode::DBE_DBCSCHAR:
    case mojo::KeyboardCode::ATTN:
    case mojo::KeyboardCode::CRSEL:
    case mojo::KeyboardCode::EXSEL:
    case mojo::KeyboardCode::EREOF:
    case mojo::KeyboardCode::PLAY:
    case mojo::KeyboardCode::ZOOM:
    case mojo::KeyboardCode::NONAME:
    case mojo::KeyboardCode::PA1:
    case mojo::KeyboardCode::OEM_CLEAR:
    case mojo::KeyboardCode::UNKNOWN:
    case mojo::KeyboardCode::ALTGR:
      NOTIMPLEMENTED() << "Key code " << key_data.windows_key_code;
      break;

    default:
      LOG(WARNING) << "Unknown key code " << key_data.windows_key_code;
      break;
  }
  return std::string();
}
