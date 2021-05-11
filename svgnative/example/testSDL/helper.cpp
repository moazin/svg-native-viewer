#include <vector>
#include <fstream>
#include <string>
#include <cmath>

#include "helper.h"

#include "svgnative/SVGNativeCWrapper.h"
#include <svgnative/SVGRenderer.h>
#include <svgnative/SVGDocument.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <SDL2/SDL.h>
#include <librsvg/rsvg.h>

#include <cairo.h>
#include <svgnative/ports/cairo/CairoSVGRenderer.h>

#ifdef USE_SKIA
#include <SkData.h>
#include <SkImage.h>
#include <SkStream.h>
#include <SkSurface.h>
#include <SkCanvas.h>
#include <SkPath.h>
#include <SkPixmap.h>
#include <svgnative/ports/skia/SkiaSVGRenderer.h>
#endif

#ifdef USE_CG
#include <CoreGraphics/CoreGraphics.h>
#include <svgnative/ports/cg/CGSVGRenderer.h>
#endif

typedef enum _Renderer {
    RENDERER_LIBRSVG,
    RENDERER_SNV_CAIRO,
    RENDERER_SNV_SKIA,
    RENDERER_SNV_CG
} Renderer;

typedef enum _View {
    VIEW_FREE_HAND,
    VIEW_CMP
} View;

typedef struct _viewBox {
    double x0;
    double y0;
    double x1;
    double y1;
} Viewbox;

typedef struct _State {
    SDL_Window *window;
    SDL_Surface *sdl_surface;
    cairo_surface_t *cairo_surface;
    cairo_t *cr;
    GdkPixbuf *pixbuf;
    GdkPixbuf *saved_pixbuf;
    Viewbox viewbox;
    int width;
    int height;
    std::vector<svg_native_renderer_type_t> renderers_supported;
    std::vector<std::string> filenames;
    cairo_surface_t *d_librsvg_surface;
    cairo_t *d_librsvg_cr;
    GdkPixbuf *d_librsvg_pixbuf;
    SVGNative::Rect d_librsvg_bound;
    std::vector<SVGNative::Rect> d_librsvg_bounds;
    View view = VIEW_FREE_HAND;
    Renderer renderer = RENDERER_LIBRSVG;
    bool show_bbox = false;
    bool show_sub_bbox = false;
    bool show_diff = false;
    int current_svg_document = 0;
    std::string svg_document;
#ifdef USE_CAIRO
    cairo_surface_t *d_cairo_surface;
    cairo_t *d_cairo_cr;
    GdkPixbuf *d_cairo_pixbuf;
    SVGNative::Rect d_cairo_bound;
    std::vector<SVGNative::Rect> d_cairo_bounds;
    bool d_cairo_is_bbox_good = false;
    float d_cairo_bbox_percentage_larger = 0;
    GdkPixbuf *d_cairo_diff_pixbuf;
    float d_cairo_percentage_diff = 0;
#endif
#ifdef USE_SKIA
    SkImageInfo d_skia_image_info;
    sk_sp<SkColorSpace> d_skia_color_space;
    sk_sp<SkSurface> d_skia_surface;
    SkCanvas* d_skia_canvas;
    GdkPixbuf *d_skia_pixbuf;
    SVGNative::Rect d_skia_bound;
    std::vector<SVGNative::Rect> d_skia_bounds;
    bool d_skia_is_bbox_good = false;
    float d_skia_bbox_percentage_larger = 0;
    GdkPixbuf *d_skia_diff_pixbuf;
    float d_skia_percentage_diff = 0;
#endif
#ifdef USE_CG
    CGContextRef d_cg_context;
    CGColorSpaceRef d_cg_color_space;
    GdkPixbuf *d_cg_pixbuf;
    SVGNative::Rect d_cg_bound;
    std::vector<SVGNative::Rect> d_cg_bounds;
    bool d_cg_is_bbox_good = false;
    float d_cg_bbox_percentage_larger = 0;
    GdkPixbuf *d_cg_diff_pixbuf;
    float d_cg_percentage_diff = 0;
#endif
} _State;

int initialize(State **_state, int width, int height) {
    *_state = new _State;

    State *state = *_state;
    state->width = width;
    state->height = height;

    loadFiles(state);

    width += 500;
#ifdef USE_CAIRO
    state->renderers_supported.push_back(SVG_RENDERER_CAIRO);
#endif
#ifdef USE_SKIA
    state->renderers_supported.push_back(SVG_RENDERER_SKIA);
#endif
#ifdef USE_GDIPLUS
    state->renderers_supported.push_back(SVG_RENDERER_GDIPLUS);
#endif
#ifdef USE_CG
    state->renderers_supported.push_back(SVG_RENDERER_CG);
#endif

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "SDL failed to initialize: %s\n", SDL_GetError());
        return 1;
    }

    state->viewbox.x0 = 0;
    state->viewbox.y0 = 0;
    state->viewbox.x1 = 999;
    state->viewbox.y1 = 999;

    state->window = SDL_CreateWindow("SNV Demo Tool",
                                     0,
                                     0,
                                     width,
                                     height,
                                     SDL_WINDOW_ALLOW_HIGHDPI);
    if (state->window == NULL)
    {
        fprintf(stderr, "SDL window failed to initialize: %s\n", SDL_GetError());
        return 1;
    }

    state->sdl_surface = SDL_GetWindowSurface(state->window);
    state->cairo_surface = cairo_image_surface_create_for_data((unsigned char*)state->sdl_surface->pixels,
            CAIRO_FORMAT_RGB24,
            state->sdl_surface->w,
            state->sdl_surface->h,
            state->sdl_surface->pitch);
    state->cr = cairo_create(state->cairo_surface);
    state->pixbuf = gdk_pixbuf_new_from_data((unsigned char*)state->sdl_surface->pixels,
                                             GDK_COLORSPACE_RGB, true, 8,
                                             state->sdl_surface->w, state->sdl_surface->h,
                                             state->sdl_surface->pitch, NULL, NULL);
    cairo_rectangle(state->cr, 0, 0, 1000, 1000);
    cairo_clip(state->cr);

    {
        state->d_librsvg_surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, state->width, state->height);
        state->d_librsvg_cr = cairo_create(state->d_librsvg_surface);
        int width = cairo_image_surface_get_width(state->d_librsvg_surface);
        int height = cairo_image_surface_get_height(state->d_librsvg_surface);
        int stride = cairo_image_surface_get_stride(state->d_librsvg_surface);
        unsigned char *data = cairo_image_surface_get_data(state->d_librsvg_surface);
        state->d_librsvg_pixbuf = gdk_pixbuf_new_from_data(data, GDK_COLORSPACE_RGB, true, 8, width, height, stride, NULL, NULL);
    }

// TODO: Above code should be in this block too
#ifdef USE_CAIRO
    {
        state->d_cairo_surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, state->width, state->height);
        state->d_cairo_cr = cairo_create(state->d_cairo_surface);
        int width = cairo_image_surface_get_width(state->d_cairo_surface);
        int height = cairo_image_surface_get_height(state->d_cairo_surface);
        int stride = cairo_image_surface_get_stride(state->d_cairo_surface);
        unsigned char *data = cairo_image_surface_get_data(state->d_cairo_surface);
        state->d_cairo_pixbuf = gdk_pixbuf_new_from_data(data, GDK_COLORSPACE_RGB, true, 8, width, height, stride, NULL, NULL);
        state->d_cairo_diff_pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, true, 8, width, height);
    }
#endif

#ifdef USE_SKIA
    {
        state->d_skia_image_info = SkImageInfo::Make(state->width, state->height, kBGRA_8888_SkColorType, kPremul_SkAlphaType);
        state->d_skia_surface = SkSurface::MakeRaster(state->d_skia_image_info);
        state->d_skia_canvas= state->d_skia_surface->getCanvas();
        SkPixmap pixmap;
        state->d_skia_surface->peekPixels(&pixmap);
        int width = pixmap.width();
        int height = pixmap.height();
        int stride = pixmap.rowBytes();
        state->d_skia_pixbuf = gdk_pixbuf_new_from_data((unsigned char*)pixmap.addr(), GDK_COLORSPACE_RGB, true, 8, width, height, stride, NULL, NULL);
        state->d_skia_diff_pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, true, 8, width, height);
    }
#endif

#ifdef USE_CG
    {
        state->d_cg_color_space = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
        state->d_cg_context = CGBitmapContextCreate(NULL, state->width, state->height, 8, state->width * 4, state->d_cg_color_space, kCGImageAlphaNoneSkipLast);
        int width = CGBitmapContextGetWidth(state->d_cg_context);
        int height = CGBitmapContextGetHeight(state->d_cg_context);
        unsigned char *data = (unsigned char*)CGBitmapContextGetData(state->d_cg_context);
        int stride = CGBitmapContextGetBytesPerRow(state->d_cg_context);
        state->d_cg_pixbuf = gdk_pixbuf_new_from_data((unsigned char*)data, GDK_COLORSPACE_RGB, true, 8, width, height, stride, NULL, NULL);
        state->d_cg_diff_pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, true, 8, width, height);
    }
#endif

#ifdef GDIPLUS // TODO: Do this
#endif

    clearCanvas(state);

    loadCurrentSVG(state);
    return 0;
}

void loadCurrentSVG(State *state)
{
    if (state->current_svg_document < state->filenames.size()){
        std::ifstream svg_file(state->filenames[state->current_svg_document]);
        std::string svg_doc = "";
        std::string line;
        while (std::getline(svg_file, line)) {
            svg_doc += line;
        }
        state->svg_document = svg_doc;
        calculateBoundingBoxLibrsvg(state, state->d_librsvg_bound, state->d_librsvg_bounds);
        calculateBoundingBoxSNVCairo(state, state->d_cairo_bound, state->d_cairo_bounds);
        calculateBoundingBoxSNVSkia(state, state->d_skia_bound, state->d_skia_bounds);
        calculateBoundingBoxSNVCG(state, state->d_cg_bound, state->d_cg_bounds);

        {
            SVGNative::Rect bound = state->d_cairo_bound;
            bound = SVGNative::Rect{bound.x - 1, bound.y - 1, bound.width + 1, bound.height + 1};
            state->d_cairo_is_bbox_good = bound.contains(state->d_librsvg_bound);
            state->d_cairo_bbox_percentage_larger = 100*(bound.Area() / state->d_librsvg_bound.Area()) - 100.0;
        }
        {
            SVGNative::Rect bound = state->d_skia_bound;
            bound = SVGNative::Rect{bound.x - 1, bound.y - 1, bound.width + 1, bound.height + 1};
            state->d_skia_is_bbox_good = bound.contains(state->d_librsvg_bound);
            state->d_skia_bbox_percentage_larger = 100*(bound.Area() / state->d_librsvg_bound.Area()) - 100.0;
        }
        {
            SVGNative::Rect bound = state->d_cg_bound;
            bound = SVGNative::Rect{bound.x - 1, bound.y - 1, bound.width + 1, bound.height + 1};
            state->d_cg_is_bbox_good = bound.contains(state->d_librsvg_bound);
            state->d_cg_bbox_percentage_larger = 100*(bound.Area() / state->d_librsvg_bound.Area()) - 100.0;
        }
    }
}

void nextSVG(State *state)
{
    state->current_svg_document += 1;
    if (state->current_svg_document == state->filenames.size())
        state->current_svg_document = 0;
}

void prevSVG(State *state)
{
    state->current_svg_document -= 1;
    if (state->current_svg_document == -1)
        state->current_svg_document = state->filenames.size() - 1;
}

void loadFiles(State *state)
{
    state->filenames.push_back("./files/clipping-problem.svg");
    state->filenames.push_back("./files/paths-clipping.svg");
    state->filenames.push_back("./files/simple-stroke.svg");
}

void destroy(State *state)
{
    cairo_destroy(state->cr);
    cairo_surface_destroy(state->cairo_surface);

    cairo_destroy(state->d_librsvg_cr);
    cairo_surface_destroy(state->d_librsvg_surface);
#ifdef USE_CAIRO
    cairo_destroy(state->d_cairo_cr);
    cairo_surface_destroy(state->d_cairo_surface);
#endif

#ifdef USE_CG
    CGContextRelease(state->d_cg_context);
#endif
    SDL_DestroyWindow(state->window);
    SDL_Quit();
    delete state;
}

void clearCanvas(State *state)
{
    cairo_save(state->cr);
    cairo_identity_matrix(state->cr);
    cairo_reset_clip(state->cr);
    cairo_set_source_rgb(state->cr, 1.0, 1.0, 1.0);
    cairo_rectangle(state->cr, 0, 0, state->width, state->height);
    cairo_fill(state->cr);
    cairo_restore(state->cr);
    cairo_surface_flush(state->cairo_surface);
    SDL_UpdateWindowSurface(state->window);
}

void drawRectangle(State *state, float x0, float y0, float x1, float y1, double r, double g, double b, int line_width)
{
    cairo_set_source_rgb(state->cr, r, g, b);
    cairo_set_line_width(state->cr, line_width);
    cairo_rectangle(state->cr, x0, y0, (x1 - x0 + 1), (y1 - y0 + 1));
    cairo_close_path(state->cr);
    cairo_stroke(state->cr);
    cairo_surface_flush(state->cairo_surface);
    SDL_UpdateWindowSurface(state->window);
}

void zoomInTransform(State *state)
{
  double width_box = state->viewbox.x1 - state->viewbox.x0 + 1;
  double height_box = state->viewbox.y1 - state->viewbox.y0 + 1;
  double new_width = width_box * 0.8;
  double new_height = height_box * 0.8;
  double x_diff = (width_box - new_width)/2.0;
  double y_diff = (height_box - new_height)/2.0;
  state->viewbox.x0 = state->viewbox.x0 + x_diff;
  state->viewbox.y0 = state->viewbox.y0 + y_diff;
  state->viewbox.x1 = state->viewbox.x1 - x_diff;
  state->viewbox.y1 = state->viewbox.y1 - y_diff;
}

void resetTransform(State *state)
{
  state->viewbox.x0 = 0;
  state->viewbox.y0 = 0;
  state->viewbox.x1 = state->viewbox.x0 + state->width - 1;
  state->viewbox.y1 = state->viewbox.y0 + state->height - 1;
}

void zoomOutTransform(State *state)
{
  double width_box = state->viewbox.x1 - state->viewbox.x0 + 1;
  double height_box = state->viewbox.y1 - state->viewbox.y0 + 1;
  double new_width = width_box / 0.8;
  double new_height = height_box / 0.8;
  double x_diff = (new_width - width_box)/2.0;
  double y_diff = (new_height - height_box)/2.0;
  state->viewbox.x0 = state->viewbox.x0 - x_diff;
  state->viewbox.y0 = state->viewbox.y0 - y_diff;
  state->viewbox.x1 = state->viewbox.x1 + x_diff;
  state->viewbox.y1 = state->viewbox.y1 + y_diff;
}

void setTransform(State *state)
{
  double width_box = state->viewbox.x1 - state->viewbox.x0 + 1;
  double height_box = state->viewbox.y1 - state->viewbox.y0 + 1;
  double scale_x = state->width/width_box;
  double scale_y = state->height/height_box;

  cairo_identity_matrix(state->cr);
  cairo_scale(state->cr, scale_x, scale_y);
  cairo_translate(state->cr, -1 * state->viewbox.x0, -1 * state->viewbox.y0);

#ifdef USE_CAIRO
  cairo_identity_matrix(state->d_cairo_cr);
  cairo_scale(state->d_cairo_cr, scale_x, scale_y);
  cairo_translate(state->d_cairo_cr, -1 * state->viewbox.x0, -1 * state->viewbox.y0);

  cairo_identity_matrix(state->d_librsvg_cr);
  cairo_scale(state->d_librsvg_cr, scale_x, scale_y);
  cairo_translate(state->d_librsvg_cr, -1 * state->viewbox.x0, -1 * state->viewbox.y0);
#endif

#ifdef USE_SKIA
  state->d_skia_canvas->resetMatrix();
  state->d_skia_canvas->scale(scale_x, scale_y);
  state->d_skia_canvas->translate(-1 * state->viewbox.x0, -1 * state->viewbox.y0);
#endif

#ifdef USE_CG
  CGAffineTransform current_t = CGContextGetCTM(state->d_cg_context);
  CGAffineTransform inverse_t = CGAffineTransformInvert(current_t);
  CGContextConcatCTM(state->d_cg_context, inverse_t);
  CGAffineTransform m = {1.0, 0.0, 0.0, -1.0, 0.0, (float)state->height};
  CGContextConcatCTM(state->d_cg_context, m);
  CGContextScaleCTM(state->d_cg_context, scale_x, scale_y);
  CGContextTranslateCTM(state->d_cg_context, -1 * state->viewbox.x0, -1 * state->viewbox.y0);
#endif
}

void moveTransform(State *state, int x, int y)
{
  double width_box = state->viewbox.x1 - state->viewbox.x0 + 1;
  double height_box = state->viewbox.y1 - state->viewbox.y0 + 1;
  double dx = width_box * 0.1;
  double dy = height_box * 0.1;
  if (x == 1)
  {
    state->viewbox.x0 += dx;
    state->viewbox.x1 += dx;
  }
  if (x == -1)
  {
    state->viewbox.x0 -= dx;
    state->viewbox.x1 -= dx;
  }
  if (y == -1)
  {
    state->viewbox.y0 -= dy;
    state->viewbox.y1 -= dy;
  }
  if (y == 1)
  {
    state->viewbox.y0 += dy;
    state->viewbox.y1 += dy;
  }
}

void drawStateText(State *state)
{
    char characters[500];
    int base_x = state->width + 5;
    int base_y = 20;

    cairo_save(state->cr);
    cairo_identity_matrix(state->cr);
    cairo_reset_clip(state->cr);
    cairo_set_source_rgb(state->cr, 0.0, 0.0, 0.0);
    cairo_rectangle(state->cr, state->width, 0, 500, state->height);
    cairo_fill(state->cr);
    cairo_set_source_rgb(state->cr, 1, 1, 1);
    cairo_set_font_size(state->cr, 13);

    sprintf(characters, "AVAILABLE BACKENDS (%lu)", state->renderers_supported.size());
    cairo_move_to(state->cr, base_x, base_y);
    cairo_show_text(state->cr, characters);
    base_y += 20;

    for(unsigned long i = 0; i < state->renderers_supported.size(); i++){
        svg_native_renderer_type_t type = state->renderers_supported[i];
        std::string renderer;
        if (type == SVG_RENDERER_CAIRO){
            renderer = "Cairo";
        } else if (type == SVG_RENDERER_SKIA){
            renderer = "Skia";
        } else if (type == SVG_RENDERER_GDIPLUS){
            renderer = "Gdiplus";
        } else if (type == SVG_RENDERER_CG){
            renderer = "CoreGraphics";
        }
        sprintf(characters, "%s", renderer.c_str());
        cairo_move_to(state->cr, base_x, base_y);
        cairo_show_text(state->cr, characters);
        base_y += 20;
    }

    sprintf(characters, "Viewbox: %.3f %.3f %.3f %.3f", state->viewbox.x0, state->viewbox.y0, state->viewbox.x1, state->viewbox.y1);
    cairo_move_to(state->cr, base_x, base_y);
    cairo_show_text(state->cr, characters);
    base_y += 20;

    sprintf(characters, "File: %s", state->filenames[state->current_svg_document].c_str());
    cairo_move_to(state->cr, base_x, base_y);
    cairo_show_text(state->cr, characters);
    base_y += 20;

    if (state->renderer == RENDERER_LIBRSVG)
    {
        sprintf(characters, "Renderer: Librsvg");
    }
    else if(state->renderer == RENDERER_SNV_CAIRO)
    {
        sprintf(characters, "Renderer: SNV (Cairo)");
    }
    else if(state->renderer == RENDERER_SNV_SKIA)
    {
        sprintf(characters, "Renderer: SNV (Skia)");
    }
    else if(state->renderer == RENDERER_SNV_CG)
    {
        sprintf(characters, "Renderer: SNV (CG)");
    }
    cairo_move_to(state->cr, base_x, base_y);
    cairo_show_text(state->cr, characters);
    base_y += 20;


    if (state->show_bbox){
        sprintf(characters, "Show Bounding Box: True");
    } else {
        sprintf(characters, "Show Bounding Box: False");
    }
    cairo_move_to(state->cr, base_x, base_y);
    cairo_show_text(state->cr, characters);
    base_y += 20;

    if (state->show_sub_bbox){
        sprintf(characters, "Show Sub Bounding Boxes: True");
    } else {
        sprintf(characters, "Show Sub Bounding Boxes: False");
    }
    cairo_move_to(state->cr, base_x, base_y);
    cairo_show_text(state->cr, characters);
    base_y += 20;

    if (state->show_diff){
        sprintf(characters, "Show Diff: True");
    } else {
        sprintf(characters, "Show Diff: False");
    }
    cairo_move_to(state->cr, base_x, base_y);
    cairo_show_text(state->cr, characters);
    base_y += 20;


    SVGNative::Rect bound = state->d_librsvg_bound;
    sprintf(characters, "Librsvg Bounds: %.4f %.4f %.4f %.4f\n", bound.x, bound.y, bound.width, bound.height);
    cairo_move_to(state->cr, base_x, base_y);
    cairo_show_text(state->cr, characters);
    base_y += 20;

    bound = state->d_cairo_bound;
    sprintf(characters, "Cairo Bounds: %.4f %.4f %.4f %.4f (%s)(%.2f%%)\n", bound.x, bound.y, bound.width, bound.height,
            state->d_cairo_is_bbox_good ? "valid" : "invalid", state->d_cairo_bbox_percentage_larger);
    cairo_move_to(state->cr, base_x, base_y);
    cairo_show_text(state->cr, characters);
    base_y += 20;

    bound = state->d_skia_bound;
    sprintf(characters, "Skia Bounds: %.4f %.4f %.4f %.4f (%s)(%.2f%%)\n", bound.x, bound.y, bound.width, bound.height,
            state->d_skia_is_bbox_good ? "valid" : "invalid", state->d_skia_bbox_percentage_larger);
    cairo_move_to(state->cr, base_x, base_y);
    cairo_show_text(state->cr, characters);
    base_y += 20;

    bound = state->d_cg_bound;
    sprintf(characters, "CoreGraphics Bounds: %.4f %.4f %.4f %.4f (%s)(%.2f%%)\n", bound.x, bound.y, bound.width, bound.height,
            state->d_cg_is_bbox_good ? "valid" : "invalid", state->d_cg_bbox_percentage_larger);
    cairo_move_to(state->cr, base_x, base_y);
    cairo_show_text(state->cr, characters);
    base_y += 20;

    if (state->view == VIEW_FREE_HAND){
        sprintf(characters, "View: Freehand\n");
    } else {
        sprintf(characters, "View: Comparison\n");
    }
    cairo_move_to(state->cr, base_x, base_y);
    cairo_show_text(state->cr, characters);
    base_y += 20;

    if (state->view == VIEW_CMP){
        sprintf(characters, "SNV (Cairo) diff Librsvg: %.2f %%", state->d_cairo_percentage_diff);
        cairo_move_to(state->cr, base_x, base_y);
        cairo_show_text(state->cr, characters);
        base_y += 20;
        sprintf(characters, "SNV (Skia) diff Librsvg: %.2f %%", state->d_skia_percentage_diff);
        cairo_move_to(state->cr, base_x, base_y);
        cairo_show_text(state->cr, characters);
        base_y += 20;
        sprintf(characters, "SNV (CG) diff Librsvg: %.2f %%", state->d_cg_percentage_diff);
        cairo_move_to(state->cr, base_x, base_y);
        cairo_show_text(state->cr, characters);
        base_y += 20;
    }

    cairo_surface_flush(state->cairo_surface);
    SDL_UpdateWindowSurface(state->window);
    cairo_restore(state->cr);
}

void displayBuffer(State *state, int index)
{
    if (index == 0){
        gdk_pixbuf_scale(state->d_librsvg_pixbuf, state->pixbuf, 0, 0, state->width, state->height, 0, 0, 1, 1, GDK_INTERP_NEAREST);
    } else if(index == 1){
        gdk_pixbuf_scale(state->d_cairo_pixbuf, state->pixbuf, 0, 0, state->width, state->height, 0, 0, 1, 1, GDK_INTERP_NEAREST);
    } else if (index == 2){
        gdk_pixbuf_scale(state->d_skia_pixbuf, state->pixbuf, 0, 0, state->width, state->height, 0, 0, 1, 1, GDK_INTERP_NEAREST);
    } else if (index == 3){
        gdk_pixbuf_scale(state->d_cg_pixbuf, state->pixbuf, 0, 0, state->width, state->height, 0, 0, 1, 1, GDK_INTERP_NEAREST);
    }
}

void clearSVGDocumentLibrsvg(State *state){
    cairo_save(state->d_librsvg_cr);
    cairo_identity_matrix(state->d_librsvg_cr);
    cairo_set_source_rgb(state->d_librsvg_cr, 1.0, 1.0, 1.0);
    cairo_rectangle(state->d_librsvg_cr, 0, 0, state->width, state->height);
    cairo_fill(state->d_librsvg_cr);
    cairo_restore(state->d_librsvg_cr);
    cairo_surface_flush(state->d_librsvg_surface);
}

void clearSVGDocumentCairo(State *state){
    cairo_save(state->d_cairo_cr);
    cairo_identity_matrix(state->d_cairo_cr);
    cairo_set_source_rgb(state->d_cairo_cr, 1.0, 1.0, 1.0);
    cairo_rectangle(state->d_cairo_cr, 0, 0, state->width, state->height);
    cairo_fill(state->d_cairo_cr);
    cairo_restore(state->d_cairo_cr);
    cairo_surface_flush(state->d_cairo_surface);
}

void clearSVGDocumentSkia(State *state)
{
    state->d_skia_canvas->save();
    state->d_skia_canvas->resetMatrix();
    SkPath path;
    path.moveTo(0, 0);
    path.lineTo(state->width - 1, 0);
    path.lineTo(state->width - 1, state->height - 1);
    path.lineTo(0, state->height - 1);
    path.close();
    SkPaint paint;
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(SK_ColorWHITE);
    state->d_skia_canvas->drawPath(path, paint);
    state->d_skia_canvas->restore();
}

void clearSVGDocumentCG(State *state)
{
    CGContextSaveGState(state->d_cg_context);
    CGAffineTransform current_t = CGContextGetCTM(state->d_cg_context);
    CGAffineTransform inverse_t = CGAffineTransformInvert(current_t);
    CGContextConcatCTM(state->d_cg_context, inverse_t);
    CGContextSetRGBFillColor(state->d_cg_context, 1.0, 1.0, 1.0, 1.0);
    CGContextAddRect(state->d_cg_context, CGRect{CGPoint{0, 0}, CGSize{(float)state->width, (float)state->height}});
    CGContextFillPath(state->d_cg_context);
    CGContextRestoreGState(state->d_cg_context);
}

void clearSVGDocument(State *state)
{
    if (state->renderer == RENDERER_LIBRSVG)
    {
        clearSVGDocumentLibrsvg(state);
    }
    else if(state->renderer == RENDERER_SNV_CAIRO)
    {
        clearSVGDocumentCairo(state);
    }
    else if(state->renderer == RENDERER_SNV_SKIA)
    {
        clearSVGDocumentSkia(state);
    }
    else if(state->renderer == RENDERER_SNV_CG)
    {
        clearSVGDocumentCG(state);
    }
}

void setRenderer(State *state, int index)
{
    if (index == 0)
        state->renderer = RENDERER_LIBRSVG;
    else if(index == 1)
        state->renderer = RENDERER_SNV_CAIRO;
    else if(index == 2)
        state->renderer = RENDERER_SNV_SKIA;
    else if(index == 3)
        state->renderer = RENDERER_SNV_CG;
}

void drawSVGDocumentLibrsvg(State *state)
{
    clearSVGDocumentLibrsvg(state);
    GError *error = nullptr;
    const char *data = state->svg_document.c_str();
    long size_document = strlen(data);
    RsvgHandle *handle = rsvg_handle_new_from_data((const unsigned char*)data, size_document, &error);
    rsvg_handle_render_cairo(handle, state->d_librsvg_cr);
    g_object_unref(handle);
    cairo_surface_flush(state->d_librsvg_surface);
}

void drawSVGDocumentSNVCairo(State *state)
{
    clearSVGDocumentCairo(state);
    auto renderer = std::make_shared<SVGNative::CairoSVGRenderer>();
    std::string copy_doc = state->svg_document;
    auto doc = std::unique_ptr<SVGNative::SVGDocument>(SVGNative::SVGDocument::CreateSVGDocument(copy_doc.c_str(), renderer));
    renderer->SetCairo(state->d_cairo_cr);
    doc->Render();
    cairo_surface_flush(state->d_cairo_surface);
}

void drawSVGDocumentSNVSkia(State *state)
{
    clearSVGDocumentSkia(state);
    auto renderer = std::make_shared<SVGNative::SkiaSVGRenderer>();
    renderer->SetSkCanvas(state->d_skia_canvas);
    std::string copy_doc = state->svg_document;
    auto doc = std::unique_ptr<SVGNative::SVGDocument>(SVGNative::SVGDocument::CreateSVGDocument(copy_doc.c_str(), renderer));
    doc->Render();
    state->d_skia_surface->flush();
}

void drawSVGDocumentSNVCG(State *state)
{
    clearSVGDocumentCG(state);
    std::string copy_doc = state->svg_document;
    std::shared_ptr<SVGNative::CGSVGRenderer> renderer = std::make_shared<SVGNative::CGSVGRenderer>(SVGNative::CGSVGRenderer());
    renderer->SetGraphicsContext(state->d_cg_context);
    auto doc = std::unique_ptr<SVGNative::SVGDocument>(SVGNative::SVGDocument::CreateSVGDocument(copy_doc.c_str(), renderer));
    doc->Render();

    /*
    CGContextSaveGState(state->d_cg_context);

    CGContextTranslateCTM(state->d_cg_context, 500, 500);
    CGContextRotateCTM(state->d_cg_context, 45 * (M_PI/180.0));
    CGContextTranslateCTM(state->d_cg_context, -500, -500);

    CGContextSetAlpha(state->d_cg_context, 1.0);
    CGContextBeginTransparencyLayer(state->d_cg_context, 0);

    CGContextSetRGBFillColor(state->d_cg_context, 1.0, 0.0, 0.0, 1.0);
    CGContextBeginPath(state->d_cg_context);
    CGContextAddRect(state->d_cg_context, CGRectMake(400, 400, 200, 200));
    CGContextFillPath(state->d_cg_context);


    CGContextSetRGBStrokeColor(state->d_cg_context, 0.0, 1.0, 0.0, 1.0);
    CGContextSetLineWidth(state->d_cg_context, 50);
    CGContextSetMiterLimit(state->d_cg_context, 4);
    CGContextBeginPath(state->d_cg_context);
    CGContextAddRect(state->d_cg_context, CGRectMake(400, 400, 200, 200));
    CGContextStrokePath(state->d_cg_context);


    CGContextEndTransparencyLayer(state->d_cg_context);
    CGContextRestoreGState(state->d_cg_context);
    */

    int width = CGBitmapContextGetWidth(state->d_cg_context);
    int height = CGBitmapContextGetHeight(state->d_cg_context);
    unsigned char *data = (unsigned char*)CGBitmapContextGetData(state->d_cg_context);
    int stride = CGBitmapContextGetBytesPerRow(state->d_cg_context);
    for(int r = 0; r < height; r++){
        for(int c = 0; c < width; c++){
            int red = *(data + r*stride + c*4);
            int green = *(data + r*stride + c*4 + 1);
            int blue = *(data + r*stride + c*4 + 2);
            *(data + r*stride + c*4) = blue;
            *(data + r*stride + c*4 + 2) = red;
        }
    }
}

void drawSVGDocument(State *state)
{
    if (state->renderer == RENDERER_LIBRSVG)
    {
        drawSVGDocumentLibrsvg(state);
        displayBuffer(state, 0);
    }
    else if(state->renderer == RENDERER_SNV_CAIRO)
    {
        drawSVGDocumentSNVCairo(state);
        displayBuffer(state, 1);
    }
    else if(state->renderer == RENDERER_SNV_SKIA)
    {
        drawSVGDocumentSNVSkia(state);
        displayBuffer(state, 2);
    }
    else if(state->renderer == RENDERER_SNV_CG)
    {
        drawSVGDocumentSNVCG(state);
        displayBuffer(state, 3);
    }
}

void calculateBoundingBoxLibrsvg(State *state, SVGNative::Rect &bound, std::vector<SVGNative::Rect> &bounds)
{
    cairo_surface_t *recording_surface = cairo_recording_surface_create(CAIRO_CONTENT_COLOR, NULL);
    cairo_t *cr = cairo_create(recording_surface);
    GError *error = nullptr;
    const char *data = state->svg_document.c_str();
    long size_document = strlen(data);
    RsvgHandle *handle = rsvg_handle_new_from_data((const unsigned char*)data, size_document, &error);
    rsvg_handle_render_cairo(handle, cr);
    g_object_unref(handle);
    cairo_destroy(cr);
    cairo_surface_flush(recording_surface);

    double x0, y0, width, height;
    cairo_recording_surface_ink_extents(recording_surface, &x0, &y0, &width, &height);
    cairo_surface_destroy(recording_surface);

    bound = SVGNative::Rect{(float)x0, (float)y0, (float)width, (float)height};
}

void calculateBoundingBoxSNVCairo(State *state, SVGNative::Rect &bound, std::vector<SVGNative::Rect> &bounds)
{
    auto renderer = std::make_shared<SVGNative::CairoSVGRenderer>();
    std::string copy_doc = state->svg_document;
    auto doc = std::unique_ptr<SVGNative::SVGDocument>(SVGNative::SVGDocument::CreateSVGDocument(copy_doc.c_str(), renderer));
    renderer->SetCairo(state->d_cairo_cr);
    bound = doc->Bounds();
    bounds = doc->BoundsSub();
}

void calculateBoundingBoxSNVSkia(State *state, SVGNative::Rect &bound, std::vector<SVGNative::Rect> &bounds)
{
    auto renderer = std::make_shared<SVGNative::SkiaSVGRenderer>();
    renderer->SetSkCanvas(state->d_skia_canvas);
    std::string copy_doc = state->svg_document;
    auto doc = std::unique_ptr<SVGNative::SVGDocument>(SVGNative::SVGDocument::CreateSVGDocument(copy_doc.c_str(), renderer));
    bound = doc->Bounds();
    bounds = doc->BoundsSub();
}

void calculateBoundingBoxSNVCG(State *state, SVGNative::Rect &bound, std::vector<SVGNative::Rect> &bounds)
{
    std::string copy_doc = state->svg_document;
    std::shared_ptr<SVGNative::CGSVGRenderer> renderer = std::make_shared<SVGNative::CGSVGRenderer>(SVGNative::CGSVGRenderer());
    renderer->SetGraphicsContext(state->d_cg_context);
    auto doc = std::unique_ptr<SVGNative::SVGDocument>(SVGNative::SVGDocument::CreateSVGDocument(copy_doc.c_str(), renderer));

    bound = doc->Bounds();
    bounds = doc->BoundsSub();
}

void drawBoundingBoxes(State *state)
{
    SVGNative::Rect bound;
    std::vector<SVGNative::Rect> bounds;
    if (state->renderer == RENDERER_LIBRSVG)
    {
        bound = state->d_librsvg_bound;
        bounds = state->d_librsvg_bounds;
    }
    else if(state->renderer == RENDERER_SNV_CAIRO)
    {
        bound = state->d_cairo_bound;
        bounds = state->d_cairo_bounds;
    }
    else if(state->renderer == RENDERER_SNV_SKIA)
    {
        bound = state->d_skia_bound;
        bounds = state->d_skia_bounds;
    }
    else if(state->renderer == RENDERER_SNV_CG)
    {
        bound = state->d_cg_bound;
        bounds = state->d_cg_bounds;
    }

    if (state->show_sub_bbox){
        for(int i = 0; i < bounds.size(); i++)
        {
            cairo_set_line_width(state->cr, 1.0);
            cairo_set_source_rgb(state->cr, 0.0, 1.0, 0.0);
            cairo_rectangle(state->cr, bounds[i].x, bounds[i].y, bounds[i].width, bounds[i].height);
            cairo_stroke(state->cr);
            cairo_surface_flush(state->cairo_surface);
        }
    }

    cairo_set_line_width(state->cr, 1.0);
    cairo_set_source_rgb(state->cr, 1.0, 0.0, 0.0);
    cairo_rectangle(state->cr, bound.x, bound.y, bound.width, bound.height);
    cairo_stroke(state->cr);
    cairo_surface_flush(state->cairo_surface);
    SDL_UpdateWindowSurface(state->window);
}

void toggleDiff(State *state){
    state->show_diff = !state->show_diff;
}

void drawFrozenImage(State *state)
{
    double width_box = state->viewbox.x1 - state->viewbox.x0 + 1;
    double height_box = state->viewbox.y1 - state->viewbox.y0 + 1;
    double scale_x = state->width/width_box;
    double scale_y = state->height/height_box;
    if (state->renderer == RENDERER_LIBRSVG){
        gdk_pixbuf_scale(state->d_librsvg_pixbuf, state->pixbuf, 0, 0, state->width, state->height, -1 * state->viewbox.x0 * scale_x, -1 * state->viewbox.y0 * scale_y, scale_x, scale_y, GDK_INTERP_NEAREST);
    } else if(state->renderer == RENDERER_SNV_CAIRO){
        gdk_pixbuf_scale(state->show_diff ? state->d_cairo_diff_pixbuf : state->d_cairo_pixbuf, state->pixbuf, 0, 0, state->width, state->height, -1 * state->viewbox.x0 * scale_x, -1 * state->viewbox.y0 * scale_y, scale_x, scale_y, GDK_INTERP_NEAREST);
    } else if (state->renderer == RENDERER_SNV_SKIA){
        gdk_pixbuf_scale(state->show_diff ? state->d_skia_diff_pixbuf : state->d_skia_pixbuf, state->pixbuf, 0, 0, state->width, state->height, -1 * state->viewbox.x0 * scale_x, -1 * state->viewbox.y0 * scale_y, scale_x, scale_y, GDK_INTERP_NEAREST);
    } else if (state->renderer == RENDERER_SNV_CG){
        gdk_pixbuf_scale(state->show_diff ? state->d_cg_diff_pixbuf : state->d_cg_pixbuf, state->pixbuf, 0, 0, state->width, state->height, -1 * state->viewbox.x0 * scale_x, -1 * state->viewbox.y0 * scale_y, scale_x, scale_y, GDK_INTERP_NEAREST);
    }
}

void computeDiff(State *state, GdkPixbuf *standard, GdkPixbuf *provided, GdkPixbuf * result, float *percentage_diff)
{
    int width = gdk_pixbuf_get_width(result);
    int height = gdk_pixbuf_get_height(result);
    int stride = gdk_pixbuf_get_rowstride(result);
    unsigned char* data_result = gdk_pixbuf_get_pixels(result);
    unsigned char* data_standard = gdk_pixbuf_get_pixels(standard);
    unsigned char* data_provided = gdk_pixbuf_get_pixels(provided);
    int pixels_diff = 0;
    int pixels_total = 0;
    for(int r = 0; r < height; r++){
        for(int c = 0; c < width; c++){
            /* clearing the result */
            *(data_result + r*stride + c*4) = 255;
            *(data_result + r*stride + c*4 + 1) = 255;
            *(data_result + r*stride + c*4 + 2) = 255;
            *(data_result + r*stride + c*4 + 3) = 0;

            if (*(data_standard + r*stride + c*4) != 255 || *(data_standard + r*stride + c*4 + 1) != 255 || *(data_standard + r*stride + c*4 + 2) != 255){
                pixels_total += 1;
            }
            /* compare standard with provided putting the result in result */
            float diff =     (abs(*(data_standard + r*stride + c*4) - *(data_provided + r*stride + c*4)) +
                              abs(*(data_standard + r*stride + c*4 + 1) - *(data_provided + r*stride + c*4 + 1)) +
                              abs(*(data_standard + r*stride + c*4 + 2) - *(data_provided + r*stride + c*4 + 2)))/3.0;
            diff = (diff / 255.0) * 100.0;
            if (diff > 0){
                pixels_diff++;
                *(data_result + r*stride + c*4) = diff;
                *(data_result + r*stride + c*4 + 1) = diff;
                *(data_result + r*stride + c*4 + 2) = diff;
                *(data_result + r*stride + c*4 + 3) = diff;
            }
        }
    }
    printf("%d\n", pixels_diff);
    *percentage_diff = (((float)pixels_diff)/((float)pixels_total))*100.0;
}

void doTheDrawing(State *state)
{
    if (state->view == VIEW_FREE_HAND) {
        setTransform(state);
        drawSVGDocument(state);
        if (state->show_bbox || state->show_sub_bbox)
            drawBoundingBoxes(state);
    } else {
        drawFrozenImage(state);
    }
    drawStateText(state);
}

void toggleBoundingBox(State *state){
    state->show_bbox = !state->show_bbox;
}

void toggleSubBoundingBox(State *state){
    state->show_sub_bbox = !state->show_sub_bbox;
}

void toggleView(State *state)
{
    if (state->view == VIEW_FREE_HAND){
        state->view = VIEW_CMP;
        drawSVGDocumentLibrsvg(state);
        drawSVGDocumentSNVCairo(state);
        drawSVGDocumentSNVSkia(state);
        drawSVGDocumentSNVCG(state);
        resetTransform(state);
        computeDiff(state, state->d_librsvg_pixbuf, state->d_cairo_pixbuf, state->d_cairo_diff_pixbuf, &state->d_cairo_percentage_diff);
        computeDiff(state, state->d_librsvg_pixbuf, state->d_skia_pixbuf, state->d_skia_diff_pixbuf, &state->d_skia_percentage_diff);
        computeDiff(state, state->d_librsvg_pixbuf, state->d_cg_pixbuf, state->d_cg_diff_pixbuf, &state->d_cg_percentage_diff);
    } else {
        state->view = VIEW_FREE_HAND;
    }
}
