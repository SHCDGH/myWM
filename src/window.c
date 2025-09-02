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

    Window minimize_btn = XCreateSimpleWindow(display, titlebar,
        win_width - 2 * TITLEBAR_HEIGHT, 0,
        TITLEBAR_HEIGHT, TITLEBAR_HEIGHT,
        0,
        BlackPixel(display, DefaultScreen(display)),
        0xffff00  // yellow
    );

    XSelectInput(display, close_btn, ButtonPressMask);
    XSelectInput(display, minimize_btn, ButtonPressMask);
    XMapWindow(display, close_btn);
    XMapWindow(display, minimize_btn);
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
    XDefineCursor(display, minimize_btn, normal_cursor);
    XDefineCursor(display, resize_handle, resize_cursor);

    // Select for mouse drag events
    XSelectInput(display, resize_handle, ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
    XMapWindow(display, resize_handle);

    // After XMapWindow(...);
    managed[managed_count].frame = frame;
    managed[managed_count].client = client;
    managed[managed_count].close_btn = close_btn;
    managed[managed_count].minimize_btn = minimize_btn;
    managed[managed_count].titlebar = titlebar;
    managed[managed_count].resize_handle = resize_handle;
    managed[managed_count].minimized = 0;  // Initially not minimized
    managed[managed_count].original_width = win_width;   // Store original dimensions
    managed[managed_count].original_height = win_height;
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

void minimize_window(int window_index) {
    if (window_index < 0 || window_index >= managed_count) return;
    if (managed[window_index].minimized) return;  // Already minimized
    
    // Store current dimensions before minimizing
    XWindowAttributes client_attr;
    XGetWindowAttributes(display, managed[window_index].client, &client_attr);
    managed[window_index].original_width = client_attr.width;
    managed[window_index].original_height = client_attr.height;
    
    // Get window title and calculate text width
    char *title = NULL;
    int title_width = 100;  // Default minimum width
    
    if (XFetchName(display, managed[window_index].client, &title) && title) {
        // Create a temporary GC to measure text
        GC gc = XCreateGC(display, managed[window_index].titlebar, 0, NULL);
        
        // Get text width using XTextWidth
        XFontStruct *font = XQueryFont(display, XGContextFromGC(gc));
        if (font) {
            title_width = XTextWidth(font, title, strlen(title));
            XFreeFontInfo(NULL, font, 1);
        } else {
            // Fallback: estimate width (8 pixels per character)
            title_width = strlen(title) * 8;
        }
        
        XFreeGC(display, gc);
        XFree(title);
    }
    
    // Add padding for margins only (no buttons when minimized)
    int minimized_width = title_width + 20;  // 20px padding
    
    // Ensure minimum width
    if (minimized_width < 120) minimized_width = 120;
    
    // Hide the client window, resize handle, and buttons
    XUnmapWindow(display, managed[window_index].client);
    XUnmapWindow(display, managed[window_index].resize_handle);
    XUnmapWindow(display, managed[window_index].close_btn);
    XUnmapWindow(display, managed[window_index].minimize_btn);
    
    // Resize frame to show titlebar with dynamic width
    XResizeWindow(display, managed[window_index].frame, 
        minimized_width, TITLEBAR_HEIGHT + 2 * BORDER_WIDTH);
    
    // Resize titlebar to match (no button repositioning since they're hidden)
    XResizeWindow(display, managed[window_index].titlebar, minimized_width, TITLEBAR_HEIGHT);
    
    // Sync to ensure all operations complete
    XSync(display, False);
    
    managed[window_index].minimized = 1;
    
    // Clear drag/resize state if this window was being manipulated
    if (dragging_frame == managed[window_index].frame) {
        dragging_frame = None;
    }
    if (resizing_frame == managed[window_index].frame) {
        resizing_frame = None;
    }
}

void restore_window(int window_index) {
    if (window_index < 0 || window_index >= managed_count) return;
    if (!managed[window_index].minimized) return;  // Not minimized
    
    // Use the stored original dimensions
    int win_width = managed[window_index].original_width;
    int win_height = managed[window_index].original_height;
    
    // Show the client window and resize handle first
    XMapWindow(display, managed[window_index].client);
    XMapWindow(display, managed[window_index].resize_handle);
    XMapWindow(display, managed[window_index].close_btn);
    XMapWindow(display, managed[window_index].minimize_btn);
    
    // Restore client window size
    XResizeWindow(display, managed[window_index].client, win_width, win_height);
    
    // Restore frame to full size
    XResizeWindow(display, managed[window_index].frame,
        win_width + 2 * BORDER_WIDTH + RESIZE_HANDLE_SIZE,
        win_height + TITLEBAR_HEIGHT + 2 * BORDER_WIDTH - 2);
    
    // Restore titlebar width and button positions
    XResizeWindow(display, managed[window_index].titlebar, win_width, TITLEBAR_HEIGHT);
    XMoveWindow(display, managed[window_index].close_btn, win_width - TITLEBAR_HEIGHT, 0);
    XMoveWindow(display, managed[window_index].minimize_btn, win_width - 2 * TITLEBAR_HEIGHT, 0);
    
    // Move resize handle to correct position
    XMoveWindow(display, managed[window_index].resize_handle,
        win_width + BORDER_WIDTH,
        win_height + TITLEBAR_HEIGHT + BORDER_WIDTH - 12);
    
    // Update minimized state before setting focus
    managed[window_index].minimized = 0;
    
    // Sync to ensure all operations complete
    XSync(display, False);
    
    XRaiseWindow(display, managed[window_index].frame);
    // Only set focus after the window is properly mapped and not minimized
    XSetInputFocus(display, managed[window_index].client, RevertToParent, CurrentTime);
}