/*
Copyright 2019 Adobe. All rights reserved.
This file is licensed to you under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License. You may obtain a copy
of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under
the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR REPRESENTATIONS
OF ANY KIND, either express or implied. See the License for the specific language
governing permissions and limitations under the License.
*/

#ifndef SVGViewer_CWrapper_h
#define SVGViewer_CWrapper_h

#include "Config.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef enum snv_renderer_type_t_ {
    SNV_RENDERER_SKIA,
} snv_renderer_type_t;

typedef enum snv_status_t_ {
    SNV_OK,
    SNV_NO_RENDERER,
    SNV_INVALID_DOCUMENT
} snv_status_t;

typedef struct snv_rect_ {
  double x0;
  double y0;
  double x1;
  double y1;
} snv_rect;

typedef struct snv_t_ snv_t;

SVG_IMP_EXP snv_status_t snv_create(const char* document_string, snv_t **context);
SVG_IMP_EXP snv_status_t snv_get_bbox(snv_t *context, snv_rect *bbox);
SVG_IMP_EXP snv_status_t snv_transform_scale(snv_t *context, double scale_x, double scale_y);
SVG_IMP_EXP snv_status_t snv_transform_translate(snv_t *context, double x, double y);
SVG_IMP_EXP snv_status_t snv_transform_reset(snv_t *context);
SVG_IMP_EXP snv_status_t snv_get_viewbox(snv_t *context, int *hasViewbox, snv_rect *viewBox);
SVG_IMP_EXP snv_status_t snv_render(snv_t *context, unsigned char *data, int width, int height, int stride);
SVG_IMP_EXP void snv_destroy(snv_t *context);

#ifdef __cplusplus
}
#endif

#endif /* SVGViewer_CWrapper_h */
