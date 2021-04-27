#include <stdio.h>

#include "gtest/gtest.h"
#include "SVGRenderer.h"
#include <CairoSVGRenderer.h>

using namespace SVGNative;

TEST(cairo_renderer_tests, cairo_basic_test)
{
  CairoSVGRenderer renderer;
  std::unique_ptr<Path> path = renderer.CreatePath();
  Path *path_raw = (Path*)path.get();
  CairoSVGPath *path_cairo = static_cast<CairoSVGPath*>(path_raw);
  path_cairo->Rect(10, 10, 100, 100);
  Rect bounds = path_cairo->Bounds();
  printf("%f %f %f %f\n", bounds.x, bounds.y, bounds.width, bounds.height);
  EXPECT_EQ(bounds.x, 10);
  EXPECT_EQ(bounds.y, 10);
  EXPECT_EQ(bounds.width, 100);
  EXPECT_EQ(bounds.height, 100);
}
