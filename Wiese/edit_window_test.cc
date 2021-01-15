#include "edit_window.h"

#include "gtest/gtest.h"

TEST(SelectionPoint, OperatorLessThan) {
  EXPECT_FALSE(wiese::SelectionPoint(1, 0) < wiese::SelectionPoint(1, 0));
  EXPECT_TRUE(wiese::SelectionPoint(1, 0) < wiese::SelectionPoint(2, 0));
  EXPECT_FALSE(wiese::SelectionPoint(2, 0) < wiese::SelectionPoint(1, 0));
  EXPECT_TRUE(wiese::SelectionPoint(1, 1) < wiese::SelectionPoint(1, 2));
  EXPECT_FALSE(wiese::SelectionPoint(1, 2) < wiese::SelectionPoint(1, 1));
}

TEST(SelectionPoint, OperatorLessThanOrEqual) {
  EXPECT_TRUE(wiese::SelectionPoint(1, 0) <= wiese::SelectionPoint(1, 0));
  EXPECT_TRUE(wiese::SelectionPoint(1, 0) <= wiese::SelectionPoint(2, 0));
  EXPECT_FALSE(wiese::SelectionPoint(2, 0) <= wiese::SelectionPoint(1, 0));
  EXPECT_TRUE(wiese::SelectionPoint(1, 1) <= wiese::SelectionPoint(1, 2));
  EXPECT_FALSE(wiese::SelectionPoint(1, 2) <= wiese::SelectionPoint(1, 1));
}

TEST(SelectionPoint, OperatorGreaterThan) {
  EXPECT_FALSE(wiese::SelectionPoint(1, 0) > wiese::SelectionPoint(2, 0));
  EXPECT_TRUE(wiese::SelectionPoint(2, 0) > wiese::SelectionPoint(1, 0));
  EXPECT_FALSE(wiese::SelectionPoint(1, 1) > wiese::SelectionPoint(1, 2));
  EXPECT_TRUE(wiese::SelectionPoint(1, 2) > wiese::SelectionPoint(1, 1));
}
