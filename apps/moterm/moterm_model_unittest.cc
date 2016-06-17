// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/moterm/moterm_model.h"

#include "base/logging.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

TEST(MotermModelTest, Position) {
  MotermModel::Position def;
  EXPECT_EQ(0, def.row);
  EXPECT_EQ(0, def.column);

  MotermModel::Position pos(12, 34);
  EXPECT_EQ(12, pos.row);
  EXPECT_EQ(34, pos.column);
}

TEST(MotermModelTest, Size) {
  MotermModel::Size def;
  EXPECT_EQ(0u, def.rows);
  EXPECT_EQ(0u, def.columns);

  MotermModel::Size size(12, 34);
  EXPECT_EQ(12u, size.rows);
  EXPECT_EQ(34u, size.columns);
}

TEST(MotermModelTest, Rectangle) {
  MotermModel::Rectangle def;
  EXPECT_EQ(0, def.position.row);
  EXPECT_EQ(0, def.position.column);
  EXPECT_EQ(0u, def.size.rows);
  EXPECT_EQ(0u, def.size.columns);
  EXPECT_TRUE(def.IsEmpty());

  MotermModel::Rectangle rect1(1, 2, 34, 56);
  EXPECT_EQ(1, rect1.position.row);
  EXPECT_EQ(2, rect1.position.column);
  EXPECT_EQ(34u, rect1.size.rows);
  EXPECT_EQ(56u, rect1.size.columns);
  EXPECT_FALSE(rect1.IsEmpty());

  MotermModel::Rectangle rect2(1, 2, 0, 0);
  EXPECT_EQ(1, rect2.position.row);
  EXPECT_EQ(2, rect2.position.column);
  EXPECT_EQ(0u, rect2.size.rows);
  EXPECT_EQ(0u, rect2.size.columns);
  EXPECT_TRUE(rect2.IsEmpty());

  MotermModel::Rectangle rect3(0, 0, 1, 2);
  EXPECT_EQ(0, rect3.position.row);
  EXPECT_EQ(0, rect3.position.column);
  EXPECT_EQ(1u, rect3.size.rows);
  EXPECT_EQ(2u, rect3.size.columns);
  EXPECT_FALSE(rect3.IsEmpty());

  MotermModel::Rectangle rect4(1, 2, 3, 0);
  EXPECT_EQ(1, rect4.position.row);
  EXPECT_EQ(2, rect4.position.column);
  EXPECT_EQ(3u, rect4.size.rows);
  EXPECT_EQ(0u, rect4.size.columns);
  EXPECT_TRUE(rect4.IsEmpty());

  MotermModel::Rectangle rect5(1, 2, 0, 3);
  EXPECT_EQ(1, rect5.position.row);
  EXPECT_EQ(2, rect5.position.column);
  EXPECT_EQ(0u, rect5.size.rows);
  EXPECT_EQ(3u, rect5.size.columns);
  EXPECT_TRUE(rect5.IsEmpty());
}

TEST(MotermModelTest, Color) {
  MotermModel::Color def;
  EXPECT_EQ(0u, def.red);
  EXPECT_EQ(0u, def.green);
  EXPECT_EQ(0u, def.blue);

  MotermModel::Color color(1, 234, 56);
  EXPECT_EQ(1u, color.red);
  EXPECT_EQ(234u, color.green);
  EXPECT_EQ(56u, color.blue);
}

TEST(MotermModelTest, CharacterInfo) {
  MotermModel::CharacterInfo char_info(65, MotermModel::kAttributesBold,
                                       MotermModel::Color(12, 34, 56),
                                       MotermModel::Color(123, 45, 67));
  EXPECT_EQ(65u, char_info.code_point);
  EXPECT_EQ(MotermModel::kAttributesBold, char_info.attributes);
  EXPECT_EQ(12u, char_info.foreground_color.red);
  EXPECT_EQ(34u, char_info.foreground_color.green);
  EXPECT_EQ(56u, char_info.foreground_color.blue);
  EXPECT_EQ(123u, char_info.background_color.red);
  EXPECT_EQ(45u, char_info.background_color.green);
  EXPECT_EQ(67u, char_info.background_color.blue);
}

TEST(MotermModelTest, StateChanges) {
  MotermModel::StateChanges state_changes;
  EXPECT_FALSE(state_changes.cursor_changed);
  EXPECT_EQ(0u, state_changes.bell_count);
  EXPECT_TRUE(state_changes.dirty_rect.IsEmpty());
  EXPECT_FALSE(state_changes.IsDirty());
  // Should be the same after reset.
  state_changes.Reset();
  EXPECT_FALSE(state_changes.cursor_changed);
  EXPECT_EQ(0u, state_changes.bell_count);
  EXPECT_TRUE(state_changes.dirty_rect.IsEmpty());
  EXPECT_FALSE(state_changes.IsDirty());

  state_changes.cursor_changed = true;
  EXPECT_TRUE(state_changes.IsDirty());
  state_changes.Reset();
  EXPECT_FALSE(state_changes.cursor_changed);
  EXPECT_FALSE(state_changes.IsDirty());

  state_changes.bell_count++;
  EXPECT_TRUE(state_changes.IsDirty());
  state_changes.Reset();
  EXPECT_EQ(0u, state_changes.bell_count);
  EXPECT_FALSE(state_changes.IsDirty());

  state_changes.dirty_rect = MotermModel::Rectangle(1, 2, 34, 56);
  EXPECT_TRUE(state_changes.IsDirty());
  state_changes.Reset();
  EXPECT_TRUE(state_changes.dirty_rect.IsEmpty());
  EXPECT_FALSE(state_changes.IsDirty());
}

TEST(MotermModelTest, Basic) {
  MotermModel model(MotermModel::Size(43, 132), MotermModel::Size(25, 80),
                    nullptr);

  MotermModel::Size size = model.GetSize();
  EXPECT_EQ(25u, size.rows);
  EXPECT_EQ(80u, size.columns);

  // The cursor should start out at the upper-left (and be visible).
  MotermModel::Position cursor_pos = model.GetCursorPosition();
  EXPECT_EQ(0, cursor_pos.row);
  EXPECT_EQ(0, cursor_pos.column);
  EXPECT_TRUE(model.GetCursorVisibility());

  MotermModel::StateChanges state_changes;
  EXPECT_FALSE(state_changes.IsDirty());

  // Print "XYZ" in bright (bold) green on red.
  static const char kXYZ[] = "\x1b[1;32;41mXYZ";
  model.ProcessInput(kXYZ, sizeof(kXYZ) - 1, &state_changes);
  EXPECT_TRUE(state_changes.IsDirty());
  EXPECT_TRUE(state_changes.cursor_changed);
  EXPECT_EQ(0u, state_changes.bell_count);
  EXPECT_FALSE(state_changes.dirty_rect.IsEmpty());
  // The model has some flexibility in the size of the dirty rectangle (it may
  // over-report), but it should contain the actually-dirty part.
  EXPECT_LE(state_changes.dirty_rect.position.row, 0);
  EXPECT_LE(state_changes.dirty_rect.position.column, 0);
  EXPECT_GE(state_changes.dirty_rect.size.rows, 1u);
  EXPECT_GE(state_changes.dirty_rect.size.columns, 3u);

  // Get the 'Y'.
  MotermModel::CharacterInfo char_info =
      model.GetCharacterInfoAt(MotermModel::Position(0, 1));
  EXPECT_EQ(static_cast<uint32_t>('Y'), char_info.code_point);
  EXPECT_EQ(MotermModel::kAttributesBold, char_info.attributes);
  // The foreground should be (bright) green-ish; this is a guess at what that
  // means.
  EXPECT_GE(char_info.foreground_color.green, 100u);
  EXPECT_GE(char_info.foreground_color.green / 2,
            char_info.foreground_color.red);
  EXPECT_GE(char_info.foreground_color.green / 2,
            char_info.foreground_color.blue);
  // The background_color should be (non-bright) red-ish.
  EXPECT_GE(char_info.background_color.red, 50u);
  EXPECT_GE(char_info.background_color.red / 2,
            char_info.background_color.green);
  EXPECT_GE(char_info.background_color.red / 2,
            char_info.background_color.blue);

  state_changes.Reset();
  EXPECT_FALSE(state_changes.IsDirty());

  // Now ring the bell three times.
  static const char kBellBellBell[] = "\a\a\a";
  model.ProcessInput(kBellBellBell, sizeof(kBellBellBell) - 1, &state_changes);
  EXPECT_TRUE(state_changes.IsDirty());
  EXPECT_FALSE(state_changes.cursor_changed);
  EXPECT_EQ(3u, state_changes.bell_count);
  EXPECT_TRUE(state_changes.dirty_rect.IsEmpty());

  model.SetSize(MotermModel::Size(43, 132), false);
  size = model.GetSize();
  EXPECT_EQ(43u, size.rows);
  EXPECT_EQ(132u, size.columns);

  model.SetSize(MotermModel::Size(40, 100), true);
  size = model.GetSize();
  EXPECT_EQ(40u, size.rows);
  EXPECT_EQ(100u, size.columns);
}

TEST(MotermModelTest, ShowHideCursor) {
  MotermModel model(MotermModel::Size(43, 132), MotermModel::Size(25, 80),
                    nullptr);

  // The cursor should start visible.
  EXPECT_TRUE(model.GetCursorVisibility());

  MotermModel::StateChanges state_changes;

  // Note: A lot of sources on the web have show/hide backwards!
  static const char kHideCursor[] = "\x1b[?25l";
  model.ProcessInput(kHideCursor, sizeof(kHideCursor) - 1, &state_changes);

  EXPECT_TRUE(state_changes.IsDirty());
  EXPECT_TRUE(state_changes.cursor_changed);
  EXPECT_FALSE(model.GetCursorVisibility());

  state_changes.Reset();

  static const char kShowCursor[] = "\x1b[?25h";
  model.ProcessInput(kShowCursor, sizeof(kShowCursor) - 1, &state_changes);

  EXPECT_TRUE(state_changes.IsDirty());
  EXPECT_TRUE(state_changes.cursor_changed);
  EXPECT_TRUE(model.GetCursorVisibility());
}

class SetResetKeypadModeTestDelegate : public MotermModel::Delegate {
 public:
  SetResetKeypadModeTestDelegate() {}
  ~SetResetKeypadModeTestDelegate() override {}

  void OnResponse(const void* buf, size_t size) override { CHECK(false); }
  void OnSetKeypadMode(bool application_mode) override {
    call_count_++;
    last_application_mode_ = application_mode;
  }

  int call_count() const { return call_count_; }
  bool last_application_mode() const { return last_application_mode_; }

 private:
  int call_count_ = 0;
  bool last_application_mode_ = false;
};

TEST(MotermModelTest, SetResetKeypadMode) {
  SetResetKeypadModeTestDelegate test_delegate;

  MotermModel model(MotermModel::Size(43, 132), MotermModel::Size(25, 80),
                    &test_delegate);

  ASSERT_EQ(0, test_delegate.call_count());
  ASSERT_FALSE(test_delegate.last_application_mode());

  MotermModel::StateChanges state_changes;

  static const char kSetKeypadAppMode[] = "\x1b=";
  model.ProcessInput(kSetKeypadAppMode, sizeof(kSetKeypadAppMode) - 1,
                     &state_changes);

  EXPECT_FALSE(state_changes.IsDirty());
  EXPECT_EQ(1, test_delegate.call_count());
  EXPECT_TRUE(test_delegate.last_application_mode());

  state_changes.Reset();

  static const char kResetKeypadAppMode[] = "\x1b>";
  model.ProcessInput(kResetKeypadAppMode, sizeof(kResetKeypadAppMode) - 1,
                     &state_changes);

  EXPECT_FALSE(state_changes.IsDirty());
  EXPECT_EQ(2, test_delegate.call_count());
  EXPECT_FALSE(test_delegate.last_application_mode());
};

// TODO(vtl): Test responses.

}  // namespace
