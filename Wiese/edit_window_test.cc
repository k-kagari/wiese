#include "edit_window.h"

#include <gtest/gtest.h>

TEST(Selection, IsSinglePointReturnsTrueWhenStartIsSameToEnd) {
  EXPECT_TRUE(wiese::EditWindow::Selection(1, 1).IsSinglePoint());
}

TEST(Selection, IsSinglePointReturnsFalseWhenStartAndEndIsNotTheSame) {
  EXPECT_FALSE(wiese::EditWindow::Selection(1, 2).IsSinglePoint());
}

TEST(Selection, IsRangeReturnsFalseWhenStartIsSameToEnd) {
  EXPECT_FALSE(wiese::EditWindow::Selection(1, 1).IsRange());
}

TEST(Selection, IsRangeReturnsTrueWhenStartAndEndIsNotTheSame) {
  EXPECT_TRUE(wiese::EditWindow::Selection(1, 2).IsRange());
}

TEST(Selection, MovePointForwardAddsOneToItself) {
  auto sel = wiese::EditWindow::Selection(1, 1);
  EXPECT_EQ(2, sel.MovePointForward());
}

TEST(Selection, MovePointBackSubsOneFromItself) {
  auto sel = wiese::EditWindow::Selection(2, 2);
  EXPECT_EQ(1, sel.MovePointBack());
}

TEST(Selection, MoveStartPosBack) {
  auto sel = wiese::EditWindow::Selection(2, 3);
  EXPECT_EQ(1, sel.MoveStartPosBack());
}

TEST(Selection, MoveStartPosForward) {
  auto sel = wiese::EditWindow::Selection(2, 3);
  EXPECT_EQ(3, sel.MoveStartPosForward());
}

TEST(Selection, MoveEndPosBack) {
  auto sel = wiese::EditWindow::Selection(2, 4);
  EXPECT_EQ(3, sel.MoveEndPosBack());
}

TEST(Selection, MoveEndPosForward) {
  auto sel = wiese::EditWindow::Selection(2, 4);
  EXPECT_EQ(5, sel.MoveEndPosForward());
}

TEST(Selection2, IsSinglePointReturnsTrueWhenStartIsSameToEnd) {
  wiese::Selection2 sel(wiese::SelectionPoint(1, 1),
                        wiese::SelectionPoint(1, 1));
  EXPECT_TRUE(sel.IsSinglePoint());
}

TEST(Selection2, IsSinglePointReturnsFalseWhenStartAndEndIsNotTheSame) {
  wiese::Selection2 sel(wiese::SelectionPoint(1, 1),
                        wiese::SelectionPoint(2, 2));
  EXPECT_FALSE(sel.IsSinglePoint());
}

TEST(Selection2, IsRangeReturnsFalseWhenStartIsSameToEnd) {
  wiese::Selection2 sel(wiese::SelectionPoint(1, 1),
                        wiese::SelectionPoint(1, 1));
  EXPECT_FALSE(sel.IsRange());
}

TEST(Selection2, IsRangeReturnsTrueWhenStartAndEndIsNotTheSame) {
  wiese::Selection2 sel(wiese::SelectionPoint(1, 1),
                        wiese::SelectionPoint(2, 2));
  EXPECT_TRUE(sel.IsRange());
}
