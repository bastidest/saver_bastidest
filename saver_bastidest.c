#include <cairo/cairo-xcb.h>
#include <cairo/cairo.h>

#include <xcb/xcb.h>
#include <xcb/xproto.h>

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <sys/select.h>
#include <sys/time.h>

#include "string_set.h"

#ifdef DEBUG
#define DEBUG_PRINT(...) printf("D: " __VA_ARGS__)
#else
#define DEBUG_PRINT(...)                                                       \
  do {                                                                         \
  } while (0)
#endif

typedef struct {
  xcb_connection_t *connection;
  xcb_screen_t *screen;
  xcb_window_t window;
  xcb_visualtype_t *visual_type;
  uint32_t event_mask;
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

X11Context x11_context;

static void cairo_close_x11_surface(cairo_surface_t *sfc) {
  cairo_surface_destroy(sfc);
}

static int xcb_get_visual_type_of_screen(xcb_screen_t *screen,
                                         xcb_visualtype_t **visual_type) {
  xcb_depth_iterator_t depth_iter;

  depth_iter = xcb_screen_allowed_depths_iterator(screen);
  for (; depth_iter.rem; xcb_depth_next(&depth_iter)) {
    xcb_visualtype_iterator_t visual_iter;

    visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
    for (; visual_iter.rem; xcb_visualtype_next(&visual_iter)) {
      if (screen->root_visual == visual_iter.data->visual_id) {
        *visual_type = visual_iter.data;
        break;
      }
    }
  }
  // TODO: check for errors
  return 0;
}

static int init_x11_context(X11Context *c, unsigned int parent_window_id) {
  /* Open the connection to the X server */
  c->connection = xcb_connect(NULL, NULL);

  /* Get the first screen */
  const xcb_setup_t *setup = xcb_get_setup(c->connection);
  xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
  c->screen = iter.data;

  /* Get dimensions of the screen */
  uint16_t height = c->screen->height_in_pixels;
  uint16_t width = c->screen->width_in_pixels;

  /* Get visual type of screen */
  xcb_get_visual_type_of_screen(c->screen, &c->visual_type);

  /* Get the parent window ID if provided */
  xcb_window_t parent_window;
  if (parent_window_id) {
    printf("Attaching to parent XID %d\n", (int)parent_window_id);
    parent_window = parent_window_id;
  } else {
    parent_window = c->screen->root;
  }

  /* Setup the event mask */
  c->event_mask = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_EXPOSURE |
                  XCB_EVENT_MASK_STRUCTURE_NOTIFY;

  /* Create the window */
  c->window = xcb_generate_id(c->connection);
  xcb_create_window(c->connection,                 /* Connection          */
                    XCB_COPY_FROM_PARENT,          /* depth (same as root)*/
                    c->window,                     /* window Id           */
                    parent_window,                 /* parent window       */
                    0, 0,                          /* x, y                */
                    width, height,                 /* width, height       */
                    0,                             /* border_width        */
                    XCB_WINDOW_CLASS_INPUT_OUTPUT, /* class               */
                    c->screen->root_visual,        /* visual              */
                    XCB_CW_EVENT_MASK, /* enable the following events */
                    (const void *)&(c->event_mask));
  /* Map the window on the screen */
  xcb_map_window(c->connection, c->window);

  /* Flush all events */
  xcb_flush(c->connection);

  return 0;
}

static int destroy_x11_context(X11Context *context) {
  xcb_disconnect(context->connection);
  return 0;
}

static int create_x11_surface(cairo_surface_t **sfc, X11Context *c,
                              DrawData *draw_data) {
  draw_data->screen_size.height = c->screen->height_in_pixels;
  draw_data->screen_size.width = c->screen->width_in_pixels;
  *sfc = cairo_xcb_surface_create(c->connection, c->window, c->visual_type,
                                  draw_data->screen_size.width,
                                  draw_data->screen_size.height);
  return 0;
}

static int cairo_paint_image(cairo_t *ctx, char *path, StringSet *cache) {
  cairo_surface_t *image;
  if (string_set_get(cache, path, (void **)(&image))) {
    // cache miss
    image = cairo_image_surface_create_from_png(path);
    string_set_add(cache, path, image);
  }

  cairo_status_t status = cairo_surface_status(image);
  if (status == CAIRO_STATUS_NO_MEMORY ||
      status == CAIRO_STATUS_FILE_NOT_FOUND ||
      status == CAIRO_STATUS_READ_ERROR || status == CAIRO_STATUS_PNG_ERROR) {
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

static void cairo_paint_text(cairo_t *ctx, char *text, float font_size,
                             float pos_x, float pos_y) {
  cairo_text_extents_t extents;

  cairo_select_font_face(ctx, "Sans", CAIRO_FONT_SLANT_NORMAL,
                         CAIRO_FONT_WEIGHT_NORMAL);

  cairo_set_font_size(ctx, font_size);
  cairo_text_extents(ctx, text, &extents);

  cairo_set_source_rgba(ctx, 1, 1, 1, 1);
  cairo_move_to(ctx, pos_x, pos_y);
  cairo_show_text(ctx, text);
}

static void cairo_paint_text_primary(cairo_t *ctx, DrawData *draw_data) {
  static char time_str[64];
  assert(strftime(time_str, sizeof(time_str), draw_data->time_format_primary,
                  draw_data->current_time));

  cairo_paint_text(ctx, time_str, 100.0, draw_data->time_offset_left,
                   (float)(draw_data->screen_size.height) -
                       draw_data->time_offset_bottom - 60.0f);
}

static void cairo_paint_text_secondary(cairo_t *ctx, DrawData *draw_data) {
  static char time_str[64];
  assert(strftime(time_str, sizeof(time_str), draw_data->time_format_secondary,
                  draw_data->current_time));

  cairo_paint_text(ctx, time_str, 50.0, draw_data->time_offset_left,
                   (float)(draw_data->screen_size.height) -
                       draw_data->time_offset_bottom);
}

static void paint(X11Context *x11_context, cairo_t *ctx,
                  cairo_surface_t *cairo_surface, DrawData *draw_data) {
  DEBUG_PRINT("paint\n");

  if (cairo_paint_image(ctx, draw_data->image_path, draw_data->image_cache)) {
    fprintf(stderr, "unable to open image '%s'\n", draw_data->image_path);
  }

  struct timeval t;
  gettimeofday(&t, 0);
  time_t time = t.tv_sec;

  /* round the seconds up, if really close */
  if (t.tv_usec > 900000) {
    time++;
  }

  draw_data->current_time = localtime(&time);

  cairo_paint_text_primary(ctx, draw_data);
  cairo_paint_text_secondary(ctx, draw_data);
  cairo_surface_flush(cairo_surface);

  xcb_flush(x11_context->connection);
}

static int process_event(X11Context *x11_context, cairo_t *cairo_context,
                         cairo_surface_t *cairo_surface, DrawData *draw_data,
                         xcb_generic_event_t *event) {
  /* what is this magic bitmap? taken from the xcb events tutorial */
  switch (event->response_type & ~0x80) {
  case XCB_CONFIGURE_NOTIFY: {
    xcb_configure_notify_event_t *e = (xcb_configure_notify_event_t *)event;
    DEBUG_PRINT("ConfigureNotify width: %d, height: %d\n", e->width, e->height);
    draw_data->screen_size.height = e->height;
    draw_data->screen_size.width = e->width;
    cairo_xcb_surface_set_size(cairo_surface, e->width, e->height);
    break;
  }
  case XCB_EXPOSE:
    DEBUG_PRINT("Expose\n");
    paint(x11_context, cairo_context, cairo_surface, draw_data);
    break;
  case XCB_BUTTON_PRESS:
    DEBUG_PRINT("ButtonPress\n");
    return 1;
  default: {
    // DEBUG_PRINT("unknown event: %d\n", ev.type);
  }
  }
  return 0;
}

static int event_loop(X11Context *x11_context, cairo_t *cairo_context,
                      cairo_surface_t *cairo_surface, DrawData *draw_data) {
  xcb_generic_event_t *event;
  int done = 0;
  while (!done && (event = xcb_wait_for_event(x11_context->connection))) {
    done = process_event(x11_context, cairo_context, cairo_surface, draw_data,
                         event);
    free(event);
  }
  return 0;
}

void tock() {
  xcb_expose_event_t invalidate_event;
  invalidate_event.window = x11_context.window;
  invalidate_event.response_type = XCB_EXPOSE;
  invalidate_event.x = 0;
  invalidate_event.y = 0;
  invalidate_event.width = 0;
  invalidate_event.height = 0;
  xcb_send_event(x11_context.connection, 0, x11_context.window,
                 XCB_EVENT_MASK_EXPOSURE, (char *)&invalidate_event);

  xcb_flush(x11_context.connection);
}

static int start_timer() {
  static timer_t timer;
  static struct sigevent sigev;
  sigev.sigev_notify = SIGEV_THREAD;
  sigev.sigev_notify_function = tock;
  sigev.sigev_notify_attributes = 0;
  sigev.sigev_value.sival_ptr = &timer;
  timer_create(CLOCK_REALTIME, &sigev, &timer);

  struct itimerspec timerspec;
  timerspec.it_interval.tv_nsec = 0;
  timerspec.it_interval.tv_sec = 1;
  timerspec.it_value.tv_nsec = 0;
  timerspec.it_value.tv_sec = 1;

  /* use TIMER_ABSTIME to sync the timer to the system clock */
  timer_settime(timer, TIMER_ABSTIME, &timerspec, 0);

  return 0;
}

int main(int argc, char **argv) {
  if (argc < 2) {
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

  unsigned int parent_window_id = 0;
  char *parent_window_id_str = getenv("XSCREENSAVER_WINDOW");
  if (parent_window_id_str != NULL) {
    parent_window_id = (unsigned int)atoi(parent_window_id_str);
  }

  init_x11_context(&x11_context, parent_window_id);

  cairo_surface_t *cairo_surface;
  create_x11_surface(&cairo_surface, &x11_context, &draw_data);

  cairo_t *ctx = cairo_create(cairo_surface);

  start_timer();
  event_loop(&x11_context, ctx, cairo_surface, &draw_data);

  string_set_destroy(draw_data.image_cache);
  cairo_destroy(ctx);
  cairo_close_x11_surface(cairo_surface);

  destroy_x11_context(&x11_context);

  return 0;
}
