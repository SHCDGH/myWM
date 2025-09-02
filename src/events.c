#include "mywm.h"

void handle_button_press(XEvent *ev) {
    for (int i = 0; i < managed_count; i++) {
        if (ev->xbutton.window == managed[i].close_btn) {
            // Use the new remove function for consistency
            remove_managed_window(managed[i].client);
            break;
        } else if (ev->xbutton.window == managed[i].titlebar) {
            XRaiseWindow(display, managed[i].frame);
            XSetInputFocus(display, managed[i].client, RevertToParent, CurrentTime);
            
            dragging_frame = managed[i].frame;
            drag_start_x = ev->xbutton.x_root;
            drag_start_y = ev->xbutton.y_root;

            XWindowAttributes attr;
            XGetWindowAttributes(display, dragging_frame, &attr);
            win_start_x = attr.x;
            win_start_y = attr.y;
        } else if (ev->xbutton.window == managed[i].resize_handle) {
            resizing_frame = managed[i].frame;
            resize_start_x = ev->xbutton.x_root;
            resize_start_y = ev->xbutton.y_root;

            // Save starting size
            XWindowAttributes attr;
            XGetWindowAttributes(display, managed[i].client, &attr);
            resize_win_start_w = attr.width;
            resize_win_start_h = attr.height;
        } else if (ev->xbutton.window == managed[i].client) {
            XRaiseWindow(display, managed[i].frame);
            XSetInputFocus(display, managed[i].client, RevertToParent, CurrentTime);
        }
    }
}

void handle_motion_notify(XEvent *ev) {
    if (dragging_frame != None) {
        int dx = ev->xmotion.x_root - drag_start_x;
        int dy = ev->xmotion.y_root - drag_start_y;
        XMoveWindow(display, dragging_frame, win_start_x + dx, win_start_y + dy);
    }
    if (resizing_frame != None) {
        int i;
        for (i = 0; i < managed_count; i++) {
            if (managed[i].frame == resizing_frame) break;
        }
        if (i == managed_count) return;

        int dw = ev->xmotion.x_root - resize_start_x;
        int dh = ev->xmotion.y_root - resize_start_y;

        int new_w = resize_win_start_w + dw;
        int new_h = resize_win_start_h + dh;
        if (new_w < 50) new_w = 50;
        if (new_h < 50) new_h = 50;

        // Resize client
        XResizeWindow(display, managed[i].client, new_w, new_h);
        XResizeWindow(display, managed[i].frame,
            new_w + 2 * BORDER_WIDTH + RESIZE_HANDLE_SIZE,
            new_h + TITLEBAR_HEIGHT + 2 * BORDER_WIDTH-2
        );

        // Resize titlebar and reposition close button
        XResizeWindow(display, managed[i].titlebar, new_w, TITLEBAR_HEIGHT);
        XMoveWindow(display, managed[i].close_btn, new_w - TITLEBAR_HEIGHT, 0);

        // Move resize handle to new bottom-right
        XMoveWindow(display, managed[i].resize_handle,
            new_w + BORDER_WIDTH,
            new_h + TITLEBAR_HEIGHT + BORDER_WIDTH - 12
        );
    }
}

void handle_button_release(XEvent *ev) {
    dragging_frame = None;
    resizing_frame = None;
}

void handle_expose(XEvent *ev) {
    for (int i = 0; i < managed_count; i++) {
        if (ev->xexpose.window == managed[i].titlebar) {
            char *title = NULL;
            if (XFetchName(display, managed[i].client, &title) && title) {
                GC gc = XCreateGC(display, managed[i].titlebar, 0, NULL);
                XSetForeground(display, gc, BlackPixel(display, DefaultScreen(display)));
                XDrawString(display, managed[i].titlebar, gc, 6, 14, title, strlen(title));
                XFreeGC(display, gc);
                XFree(title);
            }
        }
    }
}

void remove_managed_window(Window client) {
    for (int i = 0; i < managed_count; i++) {
        if (managed[i].client == client) {
            // Destroy the frame and all its children
            XDestroyWindow(display, managed[i].frame);
            
            // Remove from managed list by shifting remaining windows
            for (int j = i; j < managed_count - 1; j++) {
                managed[j] = managed[j + 1];
            }
            managed_count--;
            
            // Reset drag/resize state if this window was being manipulated
            if (dragging_frame == managed[i].frame) {
                dragging_frame = None;
            }
            if (resizing_frame == managed[i].frame) {
                resizing_frame = None;
            }
            break;
        }
    }
}

void handle_destroy_notify(XEvent *ev) {
    // Handle when a client window is destroyed
    remove_managed_window(ev->xdestroywindow.window);
}

void handle_unmap_notify(XEvent *ev) {
    // Handle when a client window is unmapped (hidden)
    // This catches cases where apps close themselves
    if (ev->xunmap.event == root) {
        remove_managed_window(ev->xunmap.window);
    }
}