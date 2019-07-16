#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo/cairo-xlib.h>
#include <cairo/cairo.h>

#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  Display *display;
  Screen *screen;
  Visual *visual;
  Window window;
} X11Context;

typedef struct {
  char *image_path;
  char *time_format;
} DrawData;

void cairo_close_x11_surface(cairo_surface_t *sfc) {
  Display *dsp = cairo_xlib_surface_get_display(sfc);

  cairo_surface_destroy(sfc);
  XCloseDisplay(dsp);
}

static int init_x11_context(X11Context *c, unsigned int parent_window_id) {
  c->display = XOpenDisplay(NULL);
  c->screen = ScreenOfDisplay(c->display, 0);
  c->visual = c->screen->root_visual;

  u_int32_t width = WidthOfScreen(c->screen);
  u_int32_t height = HeightOfScreen(c->screen);

  unsigned int parent_window;
  if(parent_window_id) {
    printf("Attaching to parent XID %d\n", parent_window_id);
    parent_window = parent_window_id;
  } else {
    parent_window = RootWindow(c->display, 0);
  }

  c->window = XCreateSimpleWindow(
    c->display,
    parent_window, 0, 0,
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

static int cairo_paint_image(cairo_t *ctx, char *path) {
  cairo_surface_t *image;
  image = cairo_image_surface_create_from_png(path);

  cairo_status_t status = cairo_surface_status(image);
  if(
    status == CAIRO_STATUS_NO_MEMORY ||
    status == CAIRO_STATUS_FILE_NOT_FOUND ||
    status == CAIRO_STATUS_READ_ERROR || 
    status == CAIRO_STATUS_PNG_ERROR
    ) {
      cairo_surface_destroy(image);
      return 1;
    }

  // int w = cairo_image_surface_get_width(image);
  // int h = cairo_image_surface_get_height(image);
  // cairo_scale(ctx, 256.0 / w, 256.0 / h);

  cairo_set_source_surface(ctx, image, 0, 0);
  cairo_paint(ctx);

  cairo_surface_destroy(image);
  return 0;
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

static void paint(cairo_t *ctx, cairo_surface_t *cairo_surface, DrawData *draw_data) {
  printf("paint\n");
  static char time_str[64];

  time_t t = time(NULL);
  struct tm *tm = localtime(&t);
  assert(strftime(time_str, sizeof(time_str), draw_data->time_format, tm));

  if(cairo_paint_image(ctx, draw_data->image_path)) {
    fprintf(stderr, "unable to open image '%s'\n", draw_data->image_path);
  }

  cairo_paint_text(ctx, time_str);
  cairo_surface_flush(cairo_surface);
}

static void processEvent(X11Context *x11_context, cairo_t *cairo_context, cairo_surface_t *cairo_surface, DrawData *draw_data) {
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
    paint(cairo_context, cairo_surface, draw_data);
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
  if(argc < 2) {
    fprintf(stderr, "No background image provided, exiting...\n");
    return -1;
  }
  DrawData draw_data;
  draw_data.image_path = argv[1];
  draw_data.time_format = "%c";

  unsigned int parent_window_id = 0;
  char* parent_window_id_str = getenv("XSCREENSAVER_WINDOW");
  if(parent_window_id_str != NULL) {
    parent_window_id = atoi(parent_window_id_str);
  }

  X11Context x11_context;
  init_x11_context(&x11_context, parent_window_id);

  cairo_surface_t *cairo_surface;
  create_x11_surface(&cairo_surface, &x11_context);

  cairo_t *ctx = cairo_create(cairo_surface);

  paint(ctx, cairo_surface, &draw_data);

  while (1) {
    processEvent(&x11_context, ctx, cairo_surface, &draw_data);
  }

  cairo_destroy(ctx);
  cairo_close_x11_surface(cairo_surface);

  return 0;
}
