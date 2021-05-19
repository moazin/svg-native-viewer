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

#include "svgnative/SNVCWrapper.h"
#include "svgnative/SVGDocument.h"
#include "svgnative/SVGRenderer.h"

#ifdef USE_SKIA
#include "svgnative/ports/skia/SkiaSVGRenderer.h"
#include <SkData.h>
#include <SkImage.h>
#include <SkStream.h>
#include <SkSurface.h>
#include <SkCanvas.h>
#include <SkPath.h>
#include <SkPixmap.h>
#endif

struct snv_t_
{
    std::shared_ptr<SVGNative::SVGRenderer> mRenderer;
    std::unique_ptr<SVGNative::SVGDocument> mDocument;
    std::shared_ptr<SVGNative::Transform> mTransform;
    snv_renderer_type_t mRendererType{SNV_RENDERER_SKIA};
};


snv_status_t snv_create(const char* document_string, snv_t **context_)
{

#ifndef USE_SKIA
  return SNV_NO_RENDERER;
#endif

  snv_t *context = new snv_t_;
  *context_ = context;

  auto renderer = std::make_shared<SVGNative::SkiaSVGRenderer>();
  context->mRenderer = renderer;
  context->mRendererType = SNV_RENDERER_SKIA;
  std::string copy_doc(document_string);
  context->mDocument = SVGNative::SVGDocument::CreateSVGDocument(copy_doc.c_str(), context->mRenderer);
  if (context->mDocument == nullptr)
    return SNV_INVALID_DOCUMENT;
  context->mTransform = context->mRenderer->CreateTransform(1, 0, 0, 1, 0, 0);
}

snv_status_t snv_get_bbox(snv_t *context, snv_rect *bbox)
{
  auto skia_image_info = SkImageInfo::Make(1, 1, kBGRA_8888_SkColorType, kPremul_SkAlphaType);
  auto skia_surface = SkSurface::MakeRaster(skia_image_info);
  auto skia_canvas= skia_surface->getCanvas();
  auto renderer = std::dynamic_pointer_cast<SVGNative::SkiaSVGRenderer>(context->mRenderer);
  renderer->SetSkCanvas(skia_canvas);
  SVGNative::Rect bound = context->mDocument->Bounds(context->mTransform);
  bbox->x0 = bound.x;
  bbox->y0 = bound.y;
  bbox->x1 = bound.x + bound.width - 1;
  bbox->y1 = bound.y + bound.height - 1;
}

snv_status_t snv_transform_scale(snv_t *context, double scale_x, double scale_y)
{
  context->mTransform->Scale(scale_x, scale_y);
}

snv_status_t snv_transform_translate(snv_t *context, double x, double y)
{
  context->mTransform->Translate(x, y);
}

snv_status_t snv_transform_reset(snv_t *context)
{
  context->mTransform->Set(1, 0, 0, 1, 0, 0);
}

snv_status_t snv_get_viewbox(snv_t *context, int *hasViewbox, snv_rect *viewbox)
{
  if (context->mDocument->HasViewBox())
  {
    *hasViewbox = 1;
    SVGNative::Rect viewBox = context->mDocument->ViewBox();
    viewbox->x0 = viewBox.x;
    viewbox->y0 = viewBox.y;
    viewbox->x1 = viewBox.x + viewBox.width - 1;
    viewbox->y1 = viewBox.y + viewBox.height - 1;
  }
  else
  {
    *hasViewbox = 0;
  }
}

snv_status_t snv_render(snv_t *context, unsigned char *data, int width, int height, int stride)
{
  auto skia_image_info = SkImageInfo::Make(width, height, kBGRA_8888_SkColorType, kPremul_SkAlphaType);
  auto skia_surface = SkSurface::MakeRasterDirect(skia_image_info, data, stride);
  auto skia_canvas= skia_surface->getCanvas();
  auto transform = std::dynamic_pointer_cast<SVGNative::SkiaSVGTransform>(context->mTransform);
  skia_canvas->setMatrix(transform->mMatrix);
  auto renderer = std::dynamic_pointer_cast<SVGNative::SkiaSVGRenderer>(context->mRenderer);
  renderer->SetSkCanvas(skia_canvas);
  context->mDocument->Render();
}

void snv_destroy(snv_t *context)
{
  delete context;
}
