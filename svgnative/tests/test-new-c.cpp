#include <stdio.h>

#include "gtest/gtest.h"
#include <svgnative/SNVCWrapper.h>


TEST(test_new_c, test_new_c_basic)
{
  static const std::string gSVGString = "<svg viewBox=\"0 0 200 200\"><circle cx=\"200\" cy=\"200\" r=\"100\" fill=\"yellow\"/></svg>";
  snv_t *context;
  snv_create(gSVGString.c_str(), &context);
  snv_rect rect_bounds;
  snv_get_bbox(context, &rect_bounds);
  printf("%f %f %f %f\n", rect_bounds.x0, rect_bounds.y0, rect_bounds.x1, rect_bounds.y1);
  snv_transform_scale(context, 1.2, 1.4);
  snv_get_bbox(context, &rect_bounds);
  printf("%f %f %f %f\n", rect_bounds.x0, rect_bounds.y0, rect_bounds.x1, rect_bounds.y1);
}
