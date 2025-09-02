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
#include <dirent.h>
#include <sys/stat.h>

#define TITLEBAR_HEIGHT 20
#define BORDER_WIDTH 2
#define RESIZE_HANDLE_SIZE 10
#define MAX_WINDOWS 100

// App launcher constants
#define MAX_APPS 200
#define APP_ICON_SIZE 64
#define APP_GRID_COLS 6
#define APP_GRID_SPACING 20
#define LAUNCHER_WIDTH 600
#define LAUNCHER_HEIGHT 500

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

typedef struct {
    char name[256];
    char exec[512];
    char icon[512];
    int x, y;  // Position in launcher grid
} AppEntry;

typedef struct {
    Window window;
    Window *app_buttons;
    int visible;
    int app_count;
    AppEntry apps[MAX_APPS];
} AppLauncher;

// Global variables
extern ManagedWindow managed[MAX_WINDOWS];
extern int managed_count;
extern Display *display;
extern Window root;
extern AppLauncher launcher;

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

// Alt+Ctrl launcher state
extern int alt_pressed;
extern int ctrl_pressed;

// Function declarations
// utils.c
void set_checkerboard_background(Display *display, Window root, int size);
void spawn_terminal(void);

// window.c
void create_frame(Window client);
void minimize_window(int window_index);
void restore_window(int window_index);

// launcher.c
void init_launcher(void);
void show_launcher(void);
void hide_launcher(void);
void scan_applications(void);
void handle_launcher_click(XEvent *ev);
void handle_launcher_expose(XEvent *ev);

// events.c
void handle_button_press(XEvent *ev);
void handle_motion_notify(XEvent *ev);
void handle_button_release(XEvent *ev);
void handle_expose(XEvent *ev);
void handle_destroy_notify(XEvent *ev);
void handle_unmap_notify(XEvent *ev);
void remove_managed_window(Window client);

#endif // MYWM_H