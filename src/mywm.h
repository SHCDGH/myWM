#ifndef MYWM_H
#define MYWM_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define TITLEBAR_HEIGHT 20
#define BORDER_WIDTH 2
#define RESIZE_HANDLE_SIZE 10
#define MAX_WINDOWS 100

typedef struct {
    Window frame;
    Window client;
    Window close_btn;
    Window minimize_btn;
    Window titlebar;
    Window resize_handle;
    int minimized;  // 0 = normal, 1 = minimized
    int original_width;   // Store original dimensions for restore
    int original_height;
} ManagedWindow;

// Global variables
extern ManagedWindow managed[MAX_WINDOWS];
extern int managed_count;
extern Display *display;
extern Window root;

// Window drag/resize state
extern Window dragging_frame;
extern int drag_start_x, drag_start_y;
extern int win_start_x, win_start_y;
extern Window resizing_frame;
extern int resize_start_x, resize_start_y;
extern int resize_win_start_w, resize_win_start_h;

// Titlebar click state for minimize/restore
extern Window clicked_titlebar;
extern int click_moved;

// Function declarations
// utils.c
void set_checkerboard_background(Display *display, Window root, int size);
void spawn_terminal(void);

// window.c
void create_frame(Window client);
void minimize_window(int window_index);
void restore_window(int window_index);

// events.c
void handle_button_press(XEvent *ev);
void handle_motion_notify(XEvent *ev);
void handle_button_release(XEvent *ev);
void handle_expose(XEvent *ev);
void handle_destroy_notify(XEvent *ev);
void handle_unmap_notify(XEvent *ev);
void remove_managed_window(Window client);

#endif // MYWM_H