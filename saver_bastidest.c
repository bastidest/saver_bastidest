#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo/cairo-xlib.h>
#include <cairo/cairo.h>

#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/select.h>

#include "string_set.h"

#ifdef DEBUG
# define DEBUG_PRINT(...) printf("D: " __VA_ARGS__)
#else
# define DEBUG_PRINT(...) do {} while (0)
#endif

typedef struct {
  Display *display;
  Screen *screen;
  Visual *visual;
  Window window;
  int fd;
} X11Context;

typedef struct {
  int width;
  int height;
} ScreenSize;

typedef struct {
  char *image_path;
  StringSet *image_cache;
  char *time_format_primary;
  char *time_format_secondary;
  struct tm *current_time;
  ScreenSize screen_size;
  float time_offset_left;
  float time_offset_bottom;
} DrawData;

void cairo_close_x11_surface(cairo_surface_t *sfc) {
  Display *dsp = cairo_xlib_surface_get_display(sfc);

  cairo_surface_destroy(sfc);
  XCloseDisplay(dsp);
}

static int init_x11_context(X11Context *c, Window parent_window_id) {
  c->display = XOpenDisplay(NULL);
  c->screen = ScreenOfDisplay(c->display, 0);
  c->visual = c->screen->root_visual;
  c->fd = ConnectionNumber(c->display);

  // todo check for negative values
  unsigned int width = (unsigned int) WidthOfScreen(c->screen);
  unsigned int height = (unsigned int) HeightOfScreen(c->screen);

  Window parent_window;
  if(parent_window_id) {
    printf("Attaching to parent XID %d\n", (int)parent_window_id);
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

static int create_x11_surface(cairo_surface_t **sfc, X11Context *c, DrawData *draw_data) {
  XWindowAttributes attrs;
  XGetWindowAttributes(c->display, c->window, &attrs);
  draw_data->screen_size.height = attrs.height;
  draw_data->screen_size.width = attrs.width;
  *sfc = cairo_xlib_surface_create(c->display, c->window, c->visual, attrs.width, attrs.height);
  return 0;
}

static int cairo_paint_image(cairo_t *ctx, char *path, StringSet *cache) {
  cairo_surface_t *image;
  if(string_set_get(cache, path, (void **)(&image))) {
    // cache miss
    image = cairo_image_surface_create_from_png(path);
    string_set_add(cache, path, image);
  }

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

  // todo: destructor for images
  // cairo_surface_destroy(image);
  return 0;
}

static void cairo_paint_text(cairo_t *ctx, char *text, float font_size, float pos_x, float pos_y) {
  cairo_text_extents_t extents;

  cairo_select_font_face(ctx, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

  cairo_set_font_size(ctx, font_size);
  cairo_text_extents(ctx, text, &extents);

  cairo_set_source_rgba(ctx, 1, 1, 1, 1);
  cairo_move_to(ctx, pos_x, pos_y);
  cairo_show_text(ctx, text);
}

static void cairo_paint_text_primary(cairo_t *ctx, DrawData *draw_data) {
  static char time_str[64];
  assert(strftime(time_str, sizeof(time_str), draw_data->time_format_primary, draw_data->current_time));

  cairo_paint_text(
    ctx,
    time_str,
    100.0,
    draw_data->time_offset_left,
    (float)(draw_data->screen_size.height) - draw_data->time_offset_bottom - 60.0f
  );
}

static void cairo_paint_text_secondary(cairo_t *ctx, DrawData *draw_data) {
  static char time_str[64];
  assert(strftime(time_str, sizeof(time_str), draw_data->time_format_secondary, draw_data->current_time));

  cairo_paint_text(
    ctx,
    time_str,
    50.0,
    draw_data->time_offset_left,
    (float)(draw_data->screen_size.height) - draw_data->time_offset_bottom
  );
}

static void paint(cairo_t *ctx, cairo_surface_t *cairo_surface, DrawData *draw_data) {
  DEBUG_PRINT("paint\n");

  if(cairo_paint_image(ctx, draw_data->image_path, draw_data->image_cache)) {
    fprintf(stderr, "unable to open image '%s'\n", draw_data->image_path);
  }

  time_t t = time(NULL);
  draw_data->current_time = localtime(&t);

  cairo_paint_text_primary(ctx, draw_data);
  cairo_paint_text_secondary(ctx, draw_data);
  cairo_surface_flush(cairo_surface);
}

static void processEvent(X11Context *x11_context, cairo_t *cairo_context, cairo_surface_t *cairo_surface, DrawData *draw_data) {
  XEvent ev;
  XNextEvent(x11_context->display, &ev);
  switch (ev.type) {
  case ConfigureNotify: {
    XConfigureEvent *event;
    event = ((XConfigureEvent*) &ev);
    DEBUG_PRINT("ConfigureNotify width: %d, height: %d\n", event->width, event->height);
    draw_data->screen_size.height = event->height;
    draw_data->screen_size.width = event->width;
    cairo_xlib_surface_set_size(cairo_surface, event->width, event->height);
    break;
  }
  case Expose:
    DEBUG_PRINT("Expose\n");
    paint(cairo_context, cairo_surface, draw_data);
    break;
  case ButtonPress:
    DEBUG_PRINT("ButtonPress\n");
    exit(0);
  default: {
    // DEBUG_PRINT("unknown event: %d\n", ev.type);
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
  draw_data.time_format_primary = "%T";
  draw_data.time_format_secondary = "%A, %B %d";
  draw_data.time_offset_left = 25.0;
  draw_data.time_offset_bottom = 60.0;
  StringSet image_cache;
  draw_data.image_cache = &image_cache;
  string_set_init(draw_data.image_cache);

  Window parent_window_id = 0;
  char* parent_window_id_str = getenv("XSCREENSAVER_WINDOW");
  if(parent_window_id_str != NULL) {
    parent_window_id = (Window) atoi(parent_window_id_str);
  }

  X11Context x11_context;
  init_x11_context(&x11_context, parent_window_id);

  cairo_surface_t *cairo_surface;
  create_x11_surface(&cairo_surface, &x11_context, &draw_data);

  cairo_t *ctx = cairo_create(cairo_surface);

  while(XPending(x11_context.display)) {
    processEvent(&x11_context, ctx, cairo_surface, &draw_data);
  }
  paint(ctx, cairo_surface, &draw_data);

  fd_set events;
  struct timeval tv;

  while (1) {
    FD_ZERO(&events);
    FD_SET(x11_context.fd, &events);

    tv.tv_usec = 0;
    tv.tv_sec = 1;
    
    int fds_ready = select(x11_context.fd + 1, &events, NULL, NULL, &tv);
    if(fds_ready == 0) {
      // timer
      paint(ctx, cairo_surface, &draw_data);
    } else if(fds_ready > 0) {
      // process available events
      while(XPending(x11_context.display)) {
        processEvent(&x11_context, ctx, cairo_surface, &draw_data);
      }
    } else {
      fprintf(stderr, "event loop error\n");
    }
  }

  string_set_destroy(draw_data.image_cache);
  cairo_destroy(ctx);
  cairo_close_x11_surface(cairo_surface);

  return 0;
}