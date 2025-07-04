#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define TITLEBAR_HEIGHT 20
#define BORDER_WIDTH 2

#define MAX_WINDOWS 100

typedef struct {
    Window frame;
    Window client;
    Window close_btn;
    Window titlebar;
    Window resize_handle;
} ManagedWindow;

ManagedWindow managed[MAX_WINDOWS];
int managed_count = 0;

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
    XSelectInput(display, titlebar, ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
    

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

// Create resize handle (bottom-right corner of frame)
Window resize_handle = XCreateSimpleWindow(display, frame,
    win_width + BORDER_WIDTH - 10,
    win_height + TITLEBAR_HEIGHT + BORDER_WIDTH - 10,
    10, 10,
    0,
    BlackPixel(display, DefaultScreen(display)),
    0x888888 // light gray
);

// Select for mouse drag events
XSelectInput(display, resize_handle, ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
XMapWindow(display, resize_handle);

    
// After XMapWindow(...);
managed[managed_count].frame = frame;
managed[managed_count].client = client;
managed[managed_count].close_btn = close_btn;
managed[managed_count].titlebar = titlebar;
managed[managed_count].resize_handle = resize_handle;
managed_count++;

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

Window dragging_frame = None;
int drag_start_x = 0, drag_start_y = 0;
int win_start_x = 0, win_start_y = 0;

Window resizing_frame = None;
int resize_start_x, resize_start_y;
int resize_win_start_w, resize_win_start_h;

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
                                for (int i = 0; i < managed_count; i++) {
                                    if (ev.xbutton.window == managed[i].close_btn) {
                                        XDestroyWindow(display, managed[i].client);
                                        XDestroyWindow(display, managed[i].frame);
                                    } else if (ev.xbutton.window == managed[i].titlebar) {
                                        dragging_frame = managed[i].frame;
                                        drag_start_x = ev.xbutton.x_root;
                                        drag_start_y = ev.xbutton.y_root;
                            
                                        XWindowAttributes attr;
                                        XGetWindowAttributes(display, dragging_frame, &attr);
                                        win_start_x = attr.x;
                                        win_start_y = attr.y;
                                    } else if (ev.xbutton.window == managed[i].resize_handle) {
                                        resizing_frame = managed[i].frame;
                                        resize_start_x = ev.xbutton.x_root;
                                        resize_start_y = ev.xbutton.y_root;
                            
                                        // Save starting size
                                        XWindowAttributes attr;
                                        XGetWindowAttributes(display, managed[i].client, &attr);
                                        resize_win_start_w = attr.width;
                                        resize_win_start_h = attr.height;
                                    }
                                }
                                break;
                            
                            case MotionNotify:
                                if (dragging_frame != None) {
                                    int dx = ev.xmotion.x_root - drag_start_x;
                                    int dy = ev.xmotion.y_root - drag_start_y;
                                    XMoveWindow(display, dragging_frame, win_start_x + dx, win_start_y + dy);
                                }
                                if (resizing_frame != None) {
                                    int i;
                                    for (i = 0; i < managed_count; i++) {
                                        if (managed[i].frame == resizing_frame) break;
                                    }
                                    if (i == managed_count) break;
                            
                                    int dw = ev.xmotion.x_root - resize_start_x;
                                    int dh = ev.xmotion.y_root - resize_start_y;
                            
                                    int new_w = resize_win_start_w + dw;
                                    int new_h = resize_win_start_h + dh;
                            
                                    // Resize client
                                    XResizeWindow(display, managed[i].client, new_w, new_h);
                            
                                    // Resize frame
                                    XResizeWindow(display, managed[i].frame,
                                        new_w + 2 * BORDER_WIDTH,
                                        new_h + TITLEBAR_HEIGHT + 2 * BORDER_WIDTH
                                    );
                            
                                    // Resize titlebar and reposition close button
                                    XResizeWindow(display, managed[i].titlebar, new_w, TITLEBAR_HEIGHT);
                                    XMoveWindow(display, managed[i].close_btn, new_w - TITLEBAR_HEIGHT, 0);
                            
                                    // Move resize handle to new bottom-right
                                    XMoveWindow(display, managed[i].resize_handle,
                                        new_w + BORDER_WIDTH - 10,
                                        new_h + TITLEBAR_HEIGHT + BORDER_WIDTH - 10
                                    );
                                }
                                break;
                            
                            case ButtonRelease:
                                dragging_frame = None;
                                resizing_frame = None;
                                break;
                            
                        
              }
    }

    return 0;
}
