#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo/cairo-xlib.h>
#include <cairo/cairo.h>

#include <stdio.h>
#include <stdlib.h>

typedef struct {

} X11Context;

int cairo_check_event(cairo_surface_t *sfc, int block) {
  char keybuf[8];
  KeySym key;
  XEvent e;

  for (;;) {
    if (block || XPending(cairo_xlib_surface_get_display(sfc)))
      XNextEvent(cairo_xlib_surface_get_display(sfc), &e);
    else
      return 0;

    switch (e.type) {
    case ButtonPress:
      return -e.xbutton.button;
    case KeyPress:
      XLookupString(&e.xkey, keybuf, sizeof(keybuf), &key, NULL);
      break;
      // return key;
    case ConfigureNotify:
      printf("ConfigureNotify\n");
      break;
    default:
      fprintf(stderr, "Dropping unhandled XEevent.type = %d.\n", e.type);
    }
  }
}

/*! Open an X11 window and create a cairo surface base on that window.
 * @param x Width of window.
 * @param y Height of window.
 * @return Returns a pointer to a valid Xlib cairo surface. The function does
 * not return on error (exit(3)).
 */
cairo_surface_t *cairo_create_x11_surface0(int x, int y) {
  Display *dsp;
  Drawable da;
  int screen;
  cairo_surface_t *sfc;

  if ((dsp = XOpenDisplay(NULL)) == NULL)
    exit(1);
  screen = DefaultScreen(dsp);
  da = XCreateSimpleWindow(dsp, DefaultRootWindow(dsp), 0, 0, x, y, 0, 0, 0);
  XSelectInput(dsp, da, ButtonPressMask | KeyPressMask);
  XMapWindow(dsp, da);

  // unsigned int width, height;
  // XGetGeometry(dsp, da, NULL, NULL, NULL, &width, &height, NULL, NULL);

  XWindowAttributes window_attributes;
  XGetWindowAttributes(dsp, da, &window_attributes);

  printf("width: %d, height: %d\n", window_attributes.width,
         window_attributes.height);

  sfc = cairo_xlib_surface_create(dsp, da, DefaultVisual(dsp, screen), x, y);
  cairo_xlib_surface_set_size(sfc, x, y);

  return sfc;
}

/*! Destroy cairo Xlib surface and close X connection.
 */
void cairo_close_x11_surface(cairo_surface_t *sfc) {
  Display *dsp = cairo_xlib_surface_get_display(sfc);

  cairo_surface_destroy(sfc);
  XCloseDisplay(dsp);
}


int main(int argc, char **argv) {
  cairo_surface_t *sfc;
  cairo_t *ctx;

  sfc = cairo_create_x11_surface0(500, 500);

  ctx = cairo_create(sfc);

  cairo_surface_t *image;
  image = cairo_image_surface_create_from_png("bg1.png");

  int w = cairo_image_surface_get_width(image);
  int h = cairo_image_surface_get_height(image);

  cairo_scale(ctx, 256.0 / w, 256.0 / h);

  cairo_set_source_surface(ctx, image, 0, 0);
  cairo_paint(ctx);

  cairo_surface_destroy(image);

  cairo_text_extents_t extents;

  const char *utf8 = "cairo";
  double x, y;

  cairo_select_font_face(ctx, "Sans", CAIRO_FONT_SLANT_NORMAL,
                         CAIRO_FONT_WEIGHT_NORMAL);

  cairo_set_font_size(ctx, 100.0);
  cairo_text_extents(ctx, utf8, &extents);

  x = 25.0;
  y = 150.0;

  cairo_set_source_rgba(ctx, 1, 1, 1, 0.8);
  cairo_move_to(ctx, x, y);
  cairo_show_text(ctx, utf8);

  cairo_destroy(ctx);

  // while (1) {
  //   processEvent(display, window, ximage, width, height);
  // }

  cairo_check_event(sfc, 1);

  cairo_close_x11_surface(sfc);

  return 0;
}
