#include "mywm.h"

// Global variables definitions
ManagedWindow managed[MAX_WINDOWS];
int managed_count = 0;
Display *display;
Window root;

// Window drag/resize state
Window dragging_frame = None;
int drag_start_x = 0, drag_start_y = 0;
int win_start_x = 0, win_start_y = 0;
Window resizing_frame = None;
int resize_start_x, resize_start_y;
int resize_win_start_w, resize_win_start_h;

// Titlebar click state for minimize/restore
Window clicked_titlebar = None;
int click_moved = 0;