#include "mywm.h"

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