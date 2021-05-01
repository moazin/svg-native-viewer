#include <stdio.h>

#include "gtest/gtest.h"
#include <SVGRenderer.h>

TEST(interval_tests, basic_interval_test)
{
  SVGNative::Interval a;
  SVGNative::Interval b(10);
  SVGNative::Interval c(10, 12);

  EXPECT_EQ((bool)a, false);
  EXPECT_EQ((bool)b, true);
  EXPECT_EQ((bool)c, true);
}


TEST(interval_tests, ordering_test)
{
  SVGNative::Interval a(10, 12);
  SVGNative::Interval b(12, 10);

  EXPECT_EQ(a.Min(), 10);
  EXPECT_EQ(a.Max(), 12);
  EXPECT_EQ(b.Min(), 10);
  EXPECT_EQ(b.Max(), 12);
}

TEST(interval_tests, intersection_test)
{
  {
    SVGNative::Interval a(10, 12);
    SVGNative::Interval b(14, 15);
    SVGNative::Interval c = a & b;
    EXPECT_EQ((bool)c, false);
  }
  {
    SVGNative::Interval a(10, 15);
    SVGNative::Interval b(11, 14);
    SVGNative::Interval c = a & b;
    EXPECT_EQ((bool)c, true);
    EXPECT_EQ(c.Min(), 11);
    EXPECT_EQ(c.Max(), 14);
  }
  {
    SVGNative::Interval a(10, 15);
    SVGNative::Interval b(12, 18);
    SVGNative::Interval c = a & b;
    EXPECT_EQ((bool)c, true);
    EXPECT_EQ(c.Min(), 12);
    EXPECT_EQ(c.Max(), 15);
  }
  {
    SVGNative::Interval a(10, 15);
    SVGNative::Interval b(15, 20);
    SVGNative::Interval c = a & b;
    EXPECT_EQ((bool)c, true);
    EXPECT_EQ(c.Min(), 15);
    EXPECT_EQ(c.Max(), 15);
  }
  {
    SVGNative::Interval a(10, 15);
    SVGNative::Interval b(16, 20);
    SVGNative::Interval c = a & b;
    EXPECT_EQ((bool)c, false);
  }
  {
    SVGNative::Interval a(14, 20);
    SVGNative::Interval b(10, 15);
    SVGNative::Interval c = a & b;
    EXPECT_EQ((bool)c, true);
    EXPECT_EQ(c.Min(), 14);
    EXPECT_EQ(c.Max(), 15);
  }

}

