#include <vector>

#include "helper.h"

#include "svgnative/SVGNativeCWrapper.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <SDL2/SDL.h>

#include <cairo.h>

#ifdef USE_SKIA
#include <SkData.h>
#include <SkImage.h>
#include <SkStream.h>
#include <SkSurface.h>
#include <SkCanvas.h>
#include <SkPath.h>
#include <SkPixmap.h>
#endif

#ifdef USE_CG
#include <CoreGraphics/CoreGraphics.h>
#endif


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
    Viewbox viewbox;
    int width;
    int height;
    std::vector<svg_native_renderer_type_t> renderers_supported;
#ifdef USE_CAIRO
    cairo_surface_t *d_cairo_surface;
    cairo_t *d_cairo_cr;
    GdkPixbuf *d_cairo_pixbuf;
#endif
#ifdef USE_SKIA
  sk_sp<SkSurface> d_skia_surface;
  SkCanvas* d_skia_canvas;
  GdkPixbuf *d_skia_pixbuf;
#endif
#ifdef USE_CG
  CGContextRef d_cg_context;
  CGColorSpaceRef d_cg_color_space;
  GdkPixbuf *d_cg_pixbuf;
#endif
} _State;

int initialize(State **_state, int width, int height) {
    *_state = new _State;

    State *state = *_state;
    state->width = width;
    state->height = height;

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
    }
#endif

#ifdef USE_SKIA
    {
        state->d_skia_surface = SkSurface::MakeRasterN32Premul(state->width, state->height);
        state->d_skia_canvas= state->d_skia_surface->getCanvas();
        SkPixmap pixmap;
        state->d_skia_surface->peekPixels(&pixmap);
        int width = pixmap.width();
        int height = pixmap.height();
        int stride = pixmap.rowBytes();
        state->d_skia_pixbuf = gdk_pixbuf_new_from_data((unsigned char*)pixmap.addr(), GDK_COLORSPACE_RGB, true, 8, width, height, stride, NULL, NULL);
    }
#endif

#ifdef USE_CG
    {
        state->d_cg_color_space = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
        state->d_cg_context = CGBitmapContextCreate(NULL, state->width, state->height, 8, state->width * 4, state->d_cg_color_space, kCGImageAlphaNoneSkipFirst);
        int width = CGBitmapContextGetWidth(state->d_cg_context);
        int height = CGBitmapContextGetHeight(state->d_cg_context);
        unsigned char *data = (unsigned char*)CGBitmapContextGetData(state->d_cg_context);
        int stride = CGBitmapContextGetBytesPerRow(state->d_cg_context);
        state->d_cg_pixbuf = gdk_pixbuf_new_from_data((unsigned char*)data, GDK_COLORSPACE_RGB, true, 8, width, height, stride, NULL, NULL);
    }
#endif

#ifdef GDIPLUS // TODO: Do this
#endif

    clearCanvas(state);

    return 0;
}

void destroy(State *state)
{
    cairo_destroy(state->cr);
    cairo_surface_destroy(state->cairo_surface);

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

    cairo_surface_flush(state->cairo_surface);
    SDL_UpdateWindowSurface(state->window);
    cairo_restore(state->cr);
}


void drawCairo(State *state)
{
    cairo_save(state->d_cairo_cr);
    cairo_identity_matrix(state->d_cairo_cr);
    cairo_reset_clip(state->d_cairo_cr);
    cairo_set_source_rgb(state->d_cairo_cr, 0, 0, 0);
    cairo_rectangle(state->d_cairo_cr, 0, 0, state->width, state->height);
    cairo_fill(state->d_cairo_cr);

    cairo_set_source_rgb(state->d_cairo_cr, 0.0, 0.0, 1.0);
    cairo_set_line_width(state->d_cairo_cr, 1.0);
    cairo_rectangle(state->d_cairo_cr, 200, 200, 200, 200);
    cairo_stroke(state->d_cairo_cr);
    cairo_surface_flush(state->d_cairo_surface);
}

void drawSkia(State *state)
{
    SkPath path;
    path.moveTo(200, 200);
    path.lineTo(400, 200);
    path.lineTo(400, 400);
    path.lineTo(200, 400);
    path.close();
    SkPaint paint;
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(SK_ColorRED);
    paint.setStrokeWidth(1.0);
    state->d_skia_canvas->drawPath(path, paint);
    state->d_skia_canvas->flush();
}

void drawCoreGraphics(State *state)
{
	CGAffineTransform m = {1.0, 0.0, 0.0, -1.0, 0.0, (float)state->height};
    CGContextConcatCTM(state->d_cg_context, m);
    CGContextSetLineWidth(state->d_cg_context, 1.0);
    CGContextSetRGBStrokeColor(state->d_cg_context, 0.0, 1.0, 0.0, 1.0);
    CGContextAddRect(state->d_cg_context, CGRect{CGPoint{200, 200}, CGSize{200, 200}});
    CGContextStrokePath(state->d_cg_context);
}

void displayBuffer(State *state, int index)
{
    if (index == 0){
        gdk_pixbuf_scale(state->d_cairo_pixbuf, state->pixbuf, 0, 0, state->width, state->height, 0, 0, 1, 1, GDK_INTERP_NEAREST);
    } else if(index == 1){
        gdk_pixbuf_scale(state->d_skia_pixbuf, state->pixbuf, 0, 0, state->width, state->height, 0, 0, 1, 1, GDK_INTERP_NEAREST);
    } else if (index == 2){
        gdk_pixbuf_scale(state->d_cg_pixbuf, state->pixbuf, 0, 0, state->width, state->height, 0, 0, 1, 1, GDK_INTERP_NEAREST);
    }
    SDL_UpdateWindowSurface(state->window);
}

void drawing(State *state)
{
    //clearCanvas(state);
    //setTransform(state);

    drawCairo(state);
    drawSkia(state);
    drawCoreGraphics(state);

    drawStateText(state);
}
