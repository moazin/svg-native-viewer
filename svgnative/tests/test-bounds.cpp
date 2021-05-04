#include <stdio.h>

#include "gtest/gtest.h"
#include <svgnative/SVGRenderer.h>
#include "librsvg/rsvg.h"

TEST(bounds_tests, bounds_basic_test)
{
    GError *error = nullptr;
    RsvgHandle *handle = rsvg_handle_new_from_file("bbox-test-docs/paths.svg", &error);
}
