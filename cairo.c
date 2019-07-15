#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo/cairo-xlib.h>
#include <cairo/cairo.h>

#include <stdio.h>
#include <stdlib.h>

typedef struct {
  Display* display;
  Visual* visual;
  Window window;
} X11Context;

void cairo_close_x11_surface(cairo_surface_t *sfc) {
  Display *dsp = cairo_xlib_surface_get_display(sfc);

  cairo_surface_destroy(sfc);
  XCloseDisplay(dsp);
}

static int init_x11_context(X11Context *c) {
  u_int16_t width = 300;
  u_int32_t height = 300;
  c->display = XOpenDisplay(NULL);
  c->visual = DefaultVisual(c->display, 0);
  c->window = XCreateSimpleWindow(
    c->display,
    RootWindow(c->display, 0), 0, 0,
    width, height,
    1, 0, 0
  );
  XSelectInput(c->display, c->window, ButtonPressMask | ExposureMask | StructureNotifyMask);
  XMapWindow(c->display, c->window);
  return 0;
}

static int create_x11_surface(cairo_surface_t **sfc, X11Context *c) {
  XWindowAttributes attrs;
  XGetWindowAttributes(c->display, c->window, &attrs);
  *sfc = cairo_xlib_surface_create(c->display, c->window, c->visual, attrs.width, attrs.height);
  // cairo_xlib_surface_set_size(*sfc, width, height);
  return 0;
}

static void cairo_paint_image(cairo_t *ctx, char *path) {
  cairo_surface_t *image;
  image = cairo_image_surface_create_from_png(path);

  // int w = cairo_image_surface_get_width(image);
  // int h = cairo_image_surface_get_height(image);
  // cairo_scale(ctx, 256.0 / w, 256.0 / h);

  cairo_set_source_surface(ctx, image, 0, 0);
  cairo_paint(ctx);

  cairo_surface_destroy(image);
}

static void cairo_paint_text(cairo_t *ctx, char *text) {
  cairo_text_extents_t extents;

  double x, y;

  cairo_select_font_face(ctx, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

  cairo_set_font_size(ctx, 100.0);
  cairo_text_extents(ctx, text, &extents);

  x = 25.0;
  y = 150.0;

  cairo_set_source_rgba(ctx, 1, 1, 1, 0.8);
  cairo_move_to(ctx, x, y);
  cairo_show_text(ctx, text);
}

static void paint(cairo_t *ctx, cairo_surface_t *cairo_surface) {
  printf("repaint\n");
  cairo_paint_image(ctx, "bg1.png");
  cairo_paint_text(ctx, "Test Text");
  cairo_surface_flush(cairo_surface);
}

static void processEvent(X11Context *x11_context, cairo_t *cairo_context, cairo_surface_t *cairo_surface) {
  XEvent ev;
  XNextEvent(x11_context->display, &ev);
  switch (ev.type) {
  case ConfigureNotify: {
    XConfigureEvent *event;
    event = ((XConfigureEvent*) &ev);
    printf("ConfigureNotify width: %d, height: %d\n", event->width, event->height);
    cairo_xlib_surface_set_size(cairo_surface, event->width, event->height);
    break;
  }
  case Expose:
    printf("Expose\n");
    paint(cairo_context, cairo_surface);
    break;
  case ButtonPress:
    printf("ButtonPress\n");
    exit(0);
  default: {
    // printf("unknown event: %d\n", ev.type);
  }
  }
}

int main(int argc, char **argv) {
  X11Context x11ctx;
  init_x11_context(&x11ctx);

  cairo_surface_t *cairo_surface;
  create_x11_surface(&cairo_surface, &x11ctx);

  cairo_t *ctx = cairo_create(cairo_surface);

  paint(ctx, cairo_surface);

  while (1) {
    processEvent(&x11ctx, ctx, cairo_surface);
  }

  cairo_destroy(ctx);
  cairo_close_x11_surface(cairo_surface);

  return 0;
}
