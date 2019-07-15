#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>
#include <inttypes.h>

int main(int argc, char **argv) {
    xcb_connection_t *connection = xcb_connect (NULL, NULL);
    xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;

    uint32_t mask;
    uint32_t values[2];
    
    xcb_window_t window;
    xcb_void_cookie_t cookie;
    
    mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    values[0] = screen->white_pixel;
    values[1] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS;
    
    window = xcb_generate_id(connection);
    cookie = xcb_create_window(connection,
                    XCB_COPY_FROM_PARENT, window, screen->root,
                    0, 0, 640, 480,
                    0,
                    XCB_WINDOW_CLASS_INPUT_OUTPUT,
                    screen->root_visual,
                    mask, values);
    
    xcb_map_window(connection, window);

    xcb_flush(connection);
    sleep(1);

    xcb_disconnect (connection);
}