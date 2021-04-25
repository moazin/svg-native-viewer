#include <stdio.h>

#include "gtest/gtest.h"
#include <SVGRenderer.h>

TEST(rectangle_tests, basic_rectangle_test)
{
  SVGNative::Rect rect{10, 10, 100, 100};
  EXPECT_EQ(rect.x, 10);
  EXPECT_EQ(rect.x, 10);
  EXPECT_EQ(rect.width, 100);
  EXPECT_EQ(rect.height, 100);
}
