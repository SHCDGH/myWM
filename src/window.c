#include "mywm.h"

void create_frame(Window client) {
    XWindowAttributes attr;
    XGetWindowAttributes(display, client, &attr);
    int win_width = attr.width;
    int win_height = attr.height;
    if(win_width<50||win_height<50){
        win_width=480;
        win_height=320;
    }
    int extra_w = RESIZE_HANDLE_SIZE;
    Window frame = XCreateSimpleWindow(display, root,
        100, 100,
        win_width + 2 * BORDER_WIDTH + extra_w,
        win_height + TITLEBAR_HEIGHT + 2 * BORDER_WIDTH-2 ,
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
    XSelectInput(display, titlebar, ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask);

    GC gc = XCreateGC(display, titlebar, 0, NULL);
    XSetForeground(display, gc, BlackPixel(display, DefaultScreen(display)));

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
        win_width + BORDER_WIDTH,
        TITLEBAR_HEIGHT + win_height + BORDER_WIDTH - 12,
        RESIZE_HANDLE_SIZE, RESIZE_HANDLE_SIZE,
        1,
        BlackPixel(display, DefaultScreen(display)),
        0x888888
    );

    Cursor normal_cursor = XCreateFontCursor(display, XC_left_ptr);
    Cursor move_cursor   = XCreateFontCursor(display, XC_fleur);
    Cursor resize_cursor = XCreateFontCursor(display, XC_bottom_right_corner);
    
    XDefineCursor(display, frame, normal_cursor);
    XDefineCursor(display, titlebar, move_cursor);
    XDefineCursor(display, close_btn, normal_cursor);
    XDefineCursor(display, resize_handle, resize_cursor);

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
    XSelectInput(display, frame, SubstructureNotifyMask | ButtonPressMask);
    
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