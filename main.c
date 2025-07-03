#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    Display *display;
    Window root;
    XEvent ev;

    Window moving_window = None;
    int start_x, start_y;
    int win_x, win_y;

    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }

    root = DefaultRootWindow(display);

    // Grab Alt+Enter for launching terminals
XGrabKey(display, XKeysymToKeycode(display, XK_Return), Mod1Mask, root,
         True, GrabModeAsync, GrabModeAsync);

    // Select events to listen for
XSelectInput(display, root, SubstructureRedirectMask | SubstructureNotifyMask | KeyPressMask);
    // Spawn initial terminal
    if (fork() == 0) {
        execlp("xterm", "xterm", NULL);
        perror("execlp xterm");
        exit(1);
    }

    // Main event loop
    while (1) {
        XNextEvent(display, &ev);

        switch (ev.type) {
        case MapRequest:
            XMapWindow(display, ev.xmaprequest.window);
            break;

        case ButtonPress:
            if (ev.xbutton.subwindow != None && (ev.xbutton.state & Mod1Mask)) {
                XRaiseWindow(display, ev.xbutton.subwindow);
                moving_window = ev.xbutton.subwindow;

                XWindowAttributes attr;
                XGetWindowAttributes(display, moving_window, &attr);
                win_x = attr.x;
                win_y = attr.y;

                start_x = ev.xbutton.x_root;
                start_y = ev.xbutton.y_root;
            }
            break;

case KeyPress:
    if ((ev.xkey.state & Mod1Mask) && XLookupKeysym(&ev.xkey, 0) == XK_Return) {
        if (fork() == 0) {
            execlp("xterm", "xterm", NULL);
            perror("execlp xterm");
            exit(1);
        }
    }
    break;

        case MotionNotify:
            if (moving_window != None) {
                int dx = ev.xmotion.x_root - start_x;
                int dy = ev.xmotion.y_root - start_y;
                XMoveWindow(display, moving_window, win_x + dx, win_y + dy);
            }
            break;

        case ButtonRelease:
            moving_window = None;
            break;

        case KeyPress:
            if ((ev.xkey.state & Mod1Mask) &&
                XLookupKeysym(&ev.xkey, 0) == XK_Return) {
                if (fork() == 0) {
                    execlp("xterm", "xterm", NULL);
                    perror("execlp xterm");
                    exit(1);
                }
            }
            break;
        }
    }

    XCloseDisplay(display);
    return 0;
}
