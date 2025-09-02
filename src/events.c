#include "mywm.h"

void handle_button_press(XEvent *ev) {
    for (int i = 0; i < managed_count; i++) {
        if (ev->xbutton.window == managed[i].close_btn) {
            // Use the new remove function for consistency
            remove_managed_window(managed[i].client);
            break;
        } else if (ev->xbutton.window == managed[i].minimize_btn) {
            // Toggle minimize/restore
            if (managed[i].minimized) {
                restore_window(i);
            } else {
                minimize_window(i);
            }
            break;
        } else if (ev->xbutton.window == managed[i].titlebar) {
            // Track titlebar click for potential restore (but don't restore immediately)
            clicked_titlebar = managed[i].titlebar;
            click_moved = 0;  // Reset movement flag
            
            // Set up dragging for both minimized and normal windows
            XRaiseWindow(display, managed[i].frame);
            if (!managed[i].minimized) {
                XSetInputFocus(display, managed[i].client, RevertToParent, CurrentTime);
            }
            
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
            // Only set focus if window is not minimized
            if (!managed[i].minimized) {
                XSetInputFocus(display, managed[i].client, RevertToParent, CurrentTime);
            }
        }
    }
}

void handle_motion_notify(XEvent *ev) {
    // If we're tracking a titlebar click, mark that movement occurred
    if (clicked_titlebar != None) {
        click_moved = 1;
    }
    
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

        // Update stored dimensions for minimize/restore
        managed[i].original_width = new_w;
        managed[i].original_height = new_h;

        // Resize titlebar and reposition close and minimize buttons
        XResizeWindow(display, managed[i].titlebar, new_w, TITLEBAR_HEIGHT);
        XMoveWindow(display, managed[i].close_btn, new_w - TITLEBAR_HEIGHT, 0);
        XMoveWindow(display, managed[i].minimize_btn, new_w - 2 * TITLEBAR_HEIGHT, 0);

        // Move resize handle to new bottom-right
        XMoveWindow(display, managed[i].resize_handle,
            new_w + BORDER_WIDTH,
            new_h + TITLEBAR_HEIGHT + BORDER_WIDTH - 12
        );
    }
}

void handle_button_release(XEvent *ev) {
    // Check if we had a titlebar click without movement
    if (clicked_titlebar != None && !click_moved) {
        // Find the window that was clicked
        for (int i = 0; i < managed_count; i++) {
            if (managed[i].titlebar == clicked_titlebar && managed[i].minimized) {
                restore_window(i);
                break;
            }
        }
    }
    
    // Reset click tracking
    clicked_titlebar = None;
    click_moved = 0;
    
    // Reset drag/resize state
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
                
                if (managed[i].minimized) {
                    // Center text both horizontally and vertically for minimized windows
                    XWindowAttributes titlebar_attr;
                    XGetWindowAttributes(display, managed[i].titlebar, &titlebar_attr);
                    
                    // Get text width for horizontal centering
                    XFontStruct *font = XQueryFont(display, XGContextFromGC(gc));
                    int text_width = 0;
                    if (font) {
                        text_width = XTextWidth(font, title, strlen(title));
                        XFreeFontInfo(NULL, font, 1);
                    } else {
                        text_width = strlen(title) * 8;  // Fallback estimate
                    }
                    
                    int x = (titlebar_attr.width - text_width) / 2;
                    int y = (TITLEBAR_HEIGHT + 8) / 2;  // Center vertically (8 is approx font height)
                    
                    XDrawString(display, managed[i].titlebar, gc, x, y, title, strlen(title));
                } else {
                    // Normal positioning for non-minimized windows
                    XDrawString(display, managed[i].titlebar, gc, 6, 14, title, strlen(title));
                }
                
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

// Key combination functions
int find_focused_window(void) {
    Window focused_window;
    int revert_to;
    XGetInputFocus(display, &focused_window, &revert_to);
    
    // Find which managed window contains this focused window
    for (int i = 0; i < managed_count; i++) {
        if (managed[i].client == focused_window || 
            managed[i].frame == focused_window ||
            managed[i].titlebar == focused_window) {
            return i;
        }
    }
    return -1;  // No focused managed window found
}

void close_focused_window(void) {
    int focused_idx = find_focused_window();
    if (focused_idx >= 0) {
        remove_managed_window(managed[focused_idx].client);
    } else {
        printf("No focused window to close\n");
    }
}

void minimize_focused_window(void) {
    int focused_idx = find_focused_window();
    if (focused_idx >= 0) {
        if (!managed[focused_idx].minimized) {
            minimize_window(focused_idx);
        } else {
            printf("Window is already minimized\n");
        }
    } else {
        printf("No focused window to minimize\n");
    }
}

void toggle_maximize_focused_window(void) {
    int focused_idx = find_focused_window();
    if (focused_idx >= 0) {
        if (managed[focused_idx].minimized) {
            restore_window(focused_idx);
        } else {
            // For now, just restore if minimized, or minimize if normal
            // True maximize functionality would require storing original size
            // and expanding to screen size
            minimize_window(focused_idx);
        }
    } else {
        printf("No focused window to maximize/restore\n");
    }
}

void cycle_windows(void) {
    if (managed_count <= 1) {
        printf("Not enough windows to cycle\n");
        return;
    }
    
    // Simple cycling: find current focused window and focus the next one
    int focused_idx = find_focused_window();
    int next_idx = (focused_idx + 1) % managed_count;
    
    if (next_idx < 0) next_idx = 0;  // If no focused window, start with first
    
    // Raise and focus the next window
    if (!managed[next_idx].minimized) {
        XRaiseWindow(display, managed[next_idx].frame);
        XSetInputFocus(display, managed[next_idx].client, RevertToParent, CurrentTime);
        printf("Cycled to window %d\n", next_idx);
    } else {
        // If next window is minimized, restore it
        restore_window(next_idx);
        printf("Cycled to and restored window %d\n", next_idx);
    }
}