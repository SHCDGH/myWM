#include "mywm.h"

int main() {
    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    root = DefaultRootWindow(display);
    set_checkerboard_background(display, root, 1);

    XSelectInput(display, root, SubstructureRedirectMask | SubstructureNotifyMask | KeyPressMask);
    XGrabKey(display, XKeysymToKeycode(display, XK_Return), Mod1Mask, root,
             True, GrabModeAsync, GrabModeAsync);

    spawn_terminal();  // launch initial terminal

    XEvent ev;
    while (1) {
        XNextEvent(display, &ev);

        switch (ev.type) {
            case MapRequest:
                create_frame(ev.xmaprequest.window);
                break;

            case KeyPress:
                if ((ev.xkey.state & Mod1Mask) && XLookupKeysym(&ev.xkey, 0) == XK_Return) {
                    printf("Alt+Enter pressed — launching xterm\n");
                    fflush(stdout);
                    spawn_terminal();
                }
                break;

            case ButtonPress:
                handle_button_press(&ev);
                break;

            case MotionNotify:
                handle_motion_notify(&ev);
                break;

            case ButtonRelease:
                handle_button_release(&ev);
                break;

            case Expose:
                handle_expose(&ev);
                break;

            case DestroyNotify:
                handle_destroy_notify(&ev);
                break;

            case UnmapNotify:
                handle_unmap_notify(&ev);
                break;
        }
    }
    return 0;
}
