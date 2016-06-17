// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/converters/input_events/input_events_type_converters.h"

#if defined(USE_X11)
#include <X11/extensions/XInput2.h>
#include <X11/Xlib.h>
#endif

#include "base/time/time.h"
#include "mojo/converters/geometry/geometry_type_converters.h"
#include "mojo/converters/input_events/mojo_extended_key_event_data.h"
#include "mojo/services/input_events/interfaces/input_events.mojom.h"
#include "ui/events/keycodes/keyboard_codes.h"

namespace mojo {
namespace {

ui::EventType MojoMouseEventTypeToUIEvent(const EventPtr& event) {
  DCHECK(!event->pointer_data.is_null());
  DCHECK_EQ(PointerKind::MOUSE, event->pointer_data->kind);
  switch (event->action) {
    case EventType::POINTER_DOWN:
      return ui::ET_MOUSE_PRESSED;

    case EventType::POINTER_UP:
      return ui::ET_MOUSE_RELEASED;

    case EventType::POINTER_MOVE:
      DCHECK(event->pointer_data);
      if (event->pointer_data->horizontal_wheel != 0 ||
          event->pointer_data->vertical_wheel != 0) {
        return ui::ET_MOUSEWHEEL;
      }
      if (static_cast<uint32_t>(event->flags) &
          (static_cast<uint32_t>(EventFlags::LEFT_MOUSE_BUTTON) |
           static_cast<uint32_t>(EventFlags::MIDDLE_MOUSE_BUTTON) |
           static_cast<uint32_t>(EventFlags::RIGHT_MOUSE_BUTTON))) {
        return ui::ET_MOUSE_DRAGGED;
      }
      return ui::ET_MOUSE_MOVED;

    default:
      NOTREACHED();
  }

  return ui::ET_MOUSE_RELEASED;
}

ui::EventType MojoTouchEventTypeToUIEvent(const EventPtr& event) {
  DCHECK(!event->pointer_data.is_null());
  DCHECK_EQ(PointerKind::TOUCH, event->pointer_data->kind);
  switch (event->action) {
    case EventType::POINTER_DOWN:
      return ui::ET_TOUCH_PRESSED;

    case EventType::POINTER_UP:
      return ui::ET_TOUCH_RELEASED;

    case EventType::POINTER_MOVE:
      return ui::ET_TOUCH_MOVED;

    case EventType::POINTER_CANCEL:
      return ui::ET_TOUCH_CANCELLED;

    default:
      NOTREACHED();
  }

  return ui::ET_TOUCH_CANCELLED;
}

void SetPointerDataLocationFromEvent(const ui::LocatedEvent& located_event,
                                     PointerData* pointer_data) {
  pointer_data->x = located_event.location_f().x();
  pointer_data->y = located_event.location_f().y();
  pointer_data->screen_x = located_event.root_location_f().x();
  pointer_data->screen_y = located_event.root_location_f().y();
}

}  // namespace

COMPILE_ASSERT(static_cast<int32>(EventFlags::NONE) ==
               static_cast<int32>(ui::EF_NONE),
               event_flags_should_match);
COMPILE_ASSERT(static_cast<int32>(EventFlags::CAPS_LOCK_DOWN) ==
               static_cast<int32>(ui::EF_CAPS_LOCK_DOWN),
               event_flags_should_match);
COMPILE_ASSERT(static_cast<int32>(EventFlags::SHIFT_DOWN) ==
               static_cast<int32>(ui::EF_SHIFT_DOWN),
               event_flags_should_match);
COMPILE_ASSERT(static_cast<int32>(EventFlags::CONTROL_DOWN) ==
               static_cast<int32>(ui::EF_CONTROL_DOWN),
               event_flags_should_match);
COMPILE_ASSERT(static_cast<int32>(EventFlags::ALT_DOWN) ==
               static_cast<int32>(ui::EF_ALT_DOWN),
               event_flags_should_match);
COMPILE_ASSERT(static_cast<int32>(EventFlags::LEFT_MOUSE_BUTTON) ==
               static_cast<int32>(ui::EF_LEFT_MOUSE_BUTTON),
               event_flags_should_match);
COMPILE_ASSERT(static_cast<int32>(EventFlags::MIDDLE_MOUSE_BUTTON) ==
               static_cast<int32>(ui::EF_MIDDLE_MOUSE_BUTTON),
               event_flags_should_match);
COMPILE_ASSERT(static_cast<int32>(EventFlags::RIGHT_MOUSE_BUTTON) ==
               static_cast<int32>(ui::EF_RIGHT_MOUSE_BUTTON),
               event_flags_should_match);
COMPILE_ASSERT(static_cast<int32>(EventFlags::COMMAND_DOWN) ==
               static_cast<int32>(ui::EF_COMMAND_DOWN),
               event_flags_should_match);
COMPILE_ASSERT(static_cast<int32>(EventFlags::EXTENDED) ==
               static_cast<int32>(ui::EF_EXTENDED),
               event_flags_should_match);
COMPILE_ASSERT(static_cast<int32>(EventFlags::IS_SYNTHESIZED) ==
               static_cast<int32>(ui::EF_IS_SYNTHESIZED),
               event_flags_should_match);
COMPILE_ASSERT(static_cast<int32>(EventFlags::ALTGR_DOWN) ==
               static_cast<int32>(ui::EF_ALTGR_DOWN),
               event_flags_should_match);
COMPILE_ASSERT(static_cast<int32>(EventFlags::MOD3_DOWN) ==
               static_cast<int32>(ui::EF_MOD3_DOWN),
               event_flags_should_match);


// static
EventType TypeConverter<EventType, ui::EventType>::Convert(ui::EventType type) {
  switch (type) {
    case ui::ET_MOUSE_PRESSED:
    case ui::ET_TOUCH_PRESSED:
      return EventType::POINTER_DOWN;

    case ui::ET_MOUSE_DRAGGED:
    case ui::ET_MOUSE_MOVED:
    case ui::ET_MOUSE_ENTERED:
    case ui::ET_MOUSE_EXITED:
    case ui::ET_TOUCH_MOVED:
    case ui::ET_MOUSEWHEEL:
      return EventType::POINTER_MOVE;

    case ui::ET_MOUSE_RELEASED:
    case ui::ET_TOUCH_RELEASED:
      return EventType::POINTER_UP;

    case ui::ET_TOUCH_CANCELLED:
      return EventType::POINTER_CANCEL;

    case ui::ET_KEY_PRESSED:
      return EventType::KEY_PRESSED;

    case ui::ET_KEY_RELEASED:
      return EventType::KEY_RELEASED;

    default:
      break;
  }
  return EventType::UNKNOWN;
}

EventPtr TypeConverter<EventPtr, ui::Event>::Convert(const ui::Event& input) {
  const EventType type = ConvertTo<EventType>(input.type());
  if (type == EventType::UNKNOWN)
    return nullptr;

  EventPtr event = Event::New();
  event->action = type;
  event->flags = EventFlags(input.flags());
  event->time_stamp = input.time_stamp().ToInternalValue();

  PointerData pointer_data;
  if (input.IsMouseEvent()) {
    const ui::LocatedEvent* located_event =
        static_cast<const ui::LocatedEvent*>(&input);
    PointerDataPtr pointer_data(PointerData::New());
    // TODO(sky): come up with a better way to handle this.
    pointer_data->pointer_id = std::numeric_limits<int32>::max();
    pointer_data->kind = PointerKind::MOUSE;
    SetPointerDataLocationFromEvent(*located_event, pointer_data.get());
    if (input.IsMouseWheelEvent()) {
      const ui::MouseWheelEvent* wheel_event =
          static_cast<const ui::MouseWheelEvent*>(&input);
      // This conversion assumes we're using the mojo meaning of these values:
      // [-1 1].
      pointer_data->horizontal_wheel =
          static_cast<float>(wheel_event->x_offset()) / 100.0f;
      pointer_data->vertical_wheel =
          static_cast<float>(wheel_event->y_offset()) / 100.0f;
    }
    event->pointer_data = pointer_data.Pass();
  } else if (input.IsTouchEvent()) {
    const ui::TouchEvent* touch_event =
        static_cast<const ui::TouchEvent*>(&input);
    PointerDataPtr pointer_data(PointerData::New());
    pointer_data->pointer_id = touch_event->touch_id();
    pointer_data->kind = PointerKind::TOUCH;
    SetPointerDataLocationFromEvent(*touch_event, pointer_data.get());
    pointer_data->radius_major = touch_event->radius_x();
    pointer_data->radius_minor = touch_event->radius_y();
    pointer_data->pressure = touch_event->force();
    pointer_data->orientation = touch_event->rotation_angle();
    event->pointer_data = pointer_data.Pass();
  } else if (input.IsKeyEvent()) {
    const ui::KeyEvent* key_event = static_cast<const ui::KeyEvent*>(&input);
    KeyDataPtr key_data(KeyData::New());
    key_data->key_code = key_event->GetConflatedWindowsKeyCode();
    key_data->native_key_code = key_event->platform_keycode();
    key_data->is_char = key_event->is_char();
    key_data->character = key_event->GetCharacter();

    if (key_event->extended_key_event_data()) {
      const MojoExtendedKeyEventData* data =
          static_cast<const MojoExtendedKeyEventData*>(
              key_event->extended_key_event_data());
      key_data->windows_key_code = static_cast<mojo::KeyboardCode>(
          data->windows_key_code());
      key_data->text = data->text();
      key_data->unmodified_text = data->unmodified_text();
    } else {
      key_data->windows_key_code = static_cast<mojo::KeyboardCode>(
          key_event->GetLocatedWindowsKeyboardCode());
      key_data->text = key_event->GetText();
      key_data->unmodified_text = key_event->GetUnmodifiedText();
    }
    event->key_data = key_data.Pass();
  }
  return event;
}

// static
EventPtr TypeConverter<EventPtr, ui::KeyEvent>::Convert(
    const ui::KeyEvent& input) {
  return Event::From(static_cast<const ui::Event&>(input));
}

// static
scoped_ptr<ui::Event> TypeConverter<scoped_ptr<ui::Event>, EventPtr>::Convert(
    const EventPtr& input) {
  gfx::PointF location;
  gfx::PointF screen_location;
  if (!input->pointer_data.is_null()) {
    location.SetPoint(input->pointer_data->x, input->pointer_data->y);
    screen_location.SetPoint(input->pointer_data->screen_x,
                             input->pointer_data->screen_y);
  }

  switch (input->action) {
    case EventType::KEY_PRESSED:
    case EventType::KEY_RELEASED: {
      scoped_ptr<ui::KeyEvent> key_event;
      if (input->key_data->is_char) {
        // TODO(johngro) : Need to verify that input->flags (a mojom generated
        // enum) is a valid set of flags for a ui::KeyEvent
        key_event.reset(new ui::KeyEvent(
            static_cast<base::char16>(input->key_data->character),
            static_cast<ui::KeyboardCode>(
                input->key_data->key_code),
            static_cast<int32_t>(input->flags)));
      } else {
        // TODO(johngro) : Need to verify that input->flags (a mojom generated
        // enum) is a valid set of flags for a ui::KeyEvent
        key_event.reset(new ui::KeyEvent(
            input->action == EventType::KEY_PRESSED ? ui::ET_KEY_PRESSED
                                                    : ui::ET_KEY_RELEASED,

            static_cast<ui::KeyboardCode>(input->key_data->key_code),
            static_cast<int32_t>(input->flags)));
      }
      key_event->SetExtendedKeyEventData(scoped_ptr<ui::ExtendedKeyEventData>(
          new MojoExtendedKeyEventData(
              static_cast<int32_t>(input->key_data->windows_key_code),
              input->key_data->text,
              input->key_data->unmodified_text)));
      key_event->set_platform_keycode(input->key_data->native_key_code);
      return key_event.Pass();
    }
    case EventType::POINTER_DOWN:
    case EventType::POINTER_UP:
    case EventType::POINTER_MOVE:
    case EventType::POINTER_CANCEL: {
      if (input->pointer_data->kind == PointerKind::MOUSE) {
        // TODO: last flags isn't right. Need to send changed_flags.
        scoped_ptr<ui::MouseEvent> event(new ui::MouseEvent(
            MojoMouseEventTypeToUIEvent(input), location, screen_location,
            base::TimeDelta::FromMilliseconds(input->time_stamp),
            ui::EventFlags(input->flags), ui::EventFlags(input->flags)));
        if (event->IsMouseWheelEvent()) {
          // This conversion assumes we're using the mojo meaning of these
          // values: [-1 1].
          scoped_ptr<ui::MouseEvent> wheel_event(new ui::MouseWheelEvent(
              *event,
              static_cast<int>(input->pointer_data->horizontal_wheel * 100),
              static_cast<int>(input->pointer_data->vertical_wheel * 100)));
          event = wheel_event.Pass();
        }
        return event.Pass();
      }
      scoped_ptr<ui::TouchEvent> touch_event(new ui::TouchEvent(
          MojoTouchEventTypeToUIEvent(input), location,
          ui::EventFlags(input->flags), input->pointer_data->pointer_id,
          base::TimeDelta::FromInternalValue(input->time_stamp),
          input->pointer_data->radius_major, input->pointer_data->radius_minor,
          input->pointer_data->orientation, input->pointer_data->pressure));
      touch_event->set_root_location(screen_location);
      return touch_event.Pass();
    }
    default:
      NOTIMPLEMENTED();
  }
  // TODO: need to support time_stamp.
  return nullptr;
}

}  // namespace mojo
