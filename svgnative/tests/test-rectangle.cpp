#include <stdio.h>

#include "gtest/gtest.h"
#include <svgnative/SVGRenderer.h>

TEST(rectangle_tests, basic_rectangle_test)
{
  SVGNative::Rect rect{10, 10, 100, 100};
  EXPECT_EQ(rect.x, 10);
  EXPECT_EQ(rect.x, 10);
  EXPECT_EQ(rect.width, 100);
  EXPECT_EQ(rect.height, 100);
}

TEST(rectangle_tests, rectangle_bool_test)
{
  SVGNative::Rect rect_a{10, 10, 0, 1};
  SVGNative::Rect rect_b{10, 10, 1, 0};
  SVGNative::Rect rect_c{10, 10, 1, 1};
  EXPECT_EQ((bool)rect_a, false);
  EXPECT_EQ((bool)rect_b, false);
  EXPECT_EQ((bool)rect_c, true);
}

TEST(rectangle_tests, rectangle_intersection_test)
{
  {
    SVGNative::Rect rect_a{10, 10, 100, 100};
    SVGNative::Rect rect_b{60, 60, 100, 100};
    SVGNative::Rect result = rect_a & rect_b;
    EXPECT_EQ((bool)result, true);
    EXPECT_EQ(result.x, 60);
    EXPECT_EQ(result.y, 60);
    EXPECT_EQ(result.width, 50);
    EXPECT_EQ(result.height, 50);
  }
  {
    SVGNative::Rect rect_a{10, 10, 10, 10};
    SVGNative::Rect rect_b{60, 60, 10, 10};
    SVGNative::Rect result = rect_a & rect_b;
    EXPECT_EQ((bool)result, false);
  }
  {
    SVGNative::Rect rect_a{10, 10, 10, 10};
    SVGNative::Rect rect_b{19, 19, 10, 10};
    SVGNative::Rect result = rect_a & rect_b;
    EXPECT_EQ((bool)result, true);
    EXPECT_EQ(result.x, 19);
    EXPECT_EQ(result.y, 19);
    EXPECT_EQ(result.width, 1);
    EXPECT_EQ(result.height, 1);
  }
  {
    SVGNative::Rect rect_a{10, 10, 10, 10};
    SVGNative::Rect rect_b{19, 19, 10, 10};
    SVGNative::Rect result = rect_a & rect_b;
    EXPECT_EQ((bool)result, true);
    EXPECT_EQ(result.x, 19);
    EXPECT_EQ(result.y, 19);
    EXPECT_EQ(result.width, 1);
    EXPECT_EQ(result.height, 1);
  }
  {
    SVGNative::Rect rect_a{10, 10, 10, 10};
    SVGNative::Rect rect_b{20, 10, 10, 10};
    SVGNative::Rect result = rect_a & rect_b;
    EXPECT_EQ((bool)result, false);
  }
  {
    SVGNative::Rect rect_a{10, 10, 100, 100};
    SVGNative::Rect rect_b{50, 50, 40, 40};
    SVGNative::Rect result = rect_a & rect_b;
    EXPECT_EQ((bool)result, true);
    EXPECT_EQ(result.x, 50);
    EXPECT_EQ(result.y, 50);
    EXPECT_EQ(result.width, 40);
    EXPECT_EQ(result.height, 40);
  }
}

TEST(rectangle_tests, rectangle_interval_test)
{
    SVGNative::Rect rect_a{10, 10, 100, 100};
    SVGNative::Rect rect_b{10, 10, 100, 100}; // inside
    SVGNative::Rect rect_c{10, 10, 50, 50};   // inside
    SVGNative::Rect rect_d{10, 10, 101, 101}; // outside
    SVGNative::Rect rect_e{50, 50, 10, 10};   // inside
    SVGNative::Rect rect_f{9, 9, 100, 100};   // outside
    SVGNative::Rect rect_g{-10, -10, 100, 100};//outside
    SVGNative::Rect rect_h{10, 10, 101, 100}; // outside
    EXPECT_EQ(rect_a.contains(rect_b), true);
    EXPECT_EQ(rect_a.contains(rect_c), true);
    EXPECT_EQ(rect_a.contains(rect_d), false);
    EXPECT_EQ(rect_a.contains(rect_e), true);
    EXPECT_EQ(rect_a.contains(rect_f), false);
    EXPECT_EQ(rect_a.contains(rect_g), false);
    EXPECT_EQ(rect_a.contains(rect_h), false);
}
