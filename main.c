#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define TITLEBAR_HEIGHT 20
#define BORDER_WIDTH 2

Display *display;
Window root;

void set_checkerboard_background(Display *display, Window root, int size) {
    int pix_width = size * 2, pix_height = size * 2;
    Pixmap pixmap = XCreatePixmap(display, root, pix_width, pix_height,
                                  DefaultDepth(display, DefaultScreen(display)));
    GC gc = XCreateGC(display, pixmap, 0, NULL);

    XSetForeground(display, gc, WhitePixel(display, DefaultScreen(display)));
    XFillRectangle(display, pixmap, gc, 0, 0, pix_width, pix_height);

    XSetForeground(display, gc, BlackPixel(display, DefaultScreen(display)));
    XFillRectangle(display, pixmap, gc, 0, 0, size, size);
    XFillRectangle(display, pixmap, gc, size, size, size, size);

    XSetWindowBackgroundPixmap(display, root, pixmap);
    XClearWindow(display, root);
    XFreeGC(display, gc);
    XFreePixmap(display, pixmap);
}

void spawn_terminal() {
    if (fork() == 0) {
        execlp("xterm", "xterm", NULL);
        perror("execlp xterm");
        exit(1);
    }
}

void create_frame(Window client) {
    int win_width = 480;
    int win_height = 320;

    Window frame = XCreateSimpleWindow(display, root,
        100, 100,  // Position on screen
        win_width + 2 * BORDER_WIDTH,
        win_height + TITLEBAR_HEIGHT + 2 * BORDER_WIDTH,
        BORDER_WIDTH,
        BlackPixel(display, DefaultScreen(display)),
        WhitePixel(display, DefaultScreen(display))
    );

    Window titlebar = XCreateSimpleWindow(display, frame,
        0, 0,
        win_width,
        TITLEBAR_HEIGHT,
        0,
        BlackPixel(display, DefaultScreen(display)),
        WhitePixel(display, DefaultScreen(display))
    );

    Window close_btn = XCreateSimpleWindow(display, titlebar,
        win_width - TITLEBAR_HEIGHT, 0,
        TITLEBAR_HEIGHT, TITLEBAR_HEIGHT,
        0,
        BlackPixel(display, DefaultScreen(display)),
        0xff0000  // red
    );

    XSelectInput(display, close_btn, ButtonPressMask);
    XMapWindow(display, close_btn);
    XMapWindow(display, titlebar);

    // Reparent and force-resize the client
    XAddToSaveSet(display, client);
    XReparentWindow(display, client, frame, 0, TITLEBAR_HEIGHT);
    XResizeWindow(display, client, win_width, win_height);

    // Tell client its new size
    XSizeHints hints;
    memset(&hints, 0, sizeof(hints));
    hints.flags = PSize;
    hints.width = win_width;
    hints.height = win_height;
    XSetWMNormalHints(display, client, &hints);

    XMapWindow(display, client);
    XMapWindow(display, frame);
}

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
                // Close button: destroy the client window
                Window child, dummy;
                int x, y;
                unsigned int mask;
                XQueryPointer(display, ev.xbutton.window, &dummy, &child, &x, &y, &x, &y, &mask);
                if (child) {
                    printf("Close button clicked — closing window\n");
                    XDestroyWindow(display, child);
                }
                break;
        }
    }

    return 0;
}
