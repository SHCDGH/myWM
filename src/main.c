#include "mywm.h"

int main() {
    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    root = DefaultRootWindow(display);
    set_checkerboard_background(display, root, 1);

    XSelectInput(display, root, SubstructureRedirectMask | SubstructureNotifyMask | KeyPressMask | KeyReleaseMask);
    XGrabKey(display, XKeysymToKeycode(display, XK_Return), Mod1Mask, root,
             True, GrabModeAsync, GrabModeAsync);
    
    // Grab the GUI/Super/Win key for launcher
    XGrabKey(display, XKeysymToKeycode(display, XK_Super_L), 0, root,
             True, GrabModeAsync, GrabModeAsync);
    
    // Grab Alt+Ctrl for launcher (alternative binding) - using a dummy key like 'l' with both modifiers
    XGrabKey(display, XKeysymToKeycode(display, XK_l), Mod1Mask | ControlMask, root,
             True, GrabModeAsync, GrabModeAsync);
    
    // Also grab just Alt and Ctrl keys individually to detect when both are pressed
    XGrabKey(display, XKeysymToKeycode(display, XK_Alt_L), 0, root,
             True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XK_Control_L), 0, root,
             True, GrabModeAsync, GrabModeAsync);
    
    // Initialize the app launcher
    init_launcher();

    spawn_terminal();  // launch initial terminal

    XEvent ev;
    while (1) {
        XNextEvent(display, &ev);

        switch (ev.type) {
            case MapRequest:
                create_frame(ev.xmaprequest.window);
                break;

            case KeyPress:
                {
                    KeySym key = XLookupKeysym(&ev.xkey, 0);
                    
                    if ((ev.xkey.state & Mod1Mask) && key == XK_Return) {
                        printf("Alt+Enter pressed — launching xterm\n");
                        fflush(stdout);
                        spawn_terminal();
                    } else if (key == XK_Super_L) {
                        printf("GUI key pressed — toggling launcher\n");
                        fflush(stdout);
                        if (launcher.visible) {
                            hide_launcher();
                        } else {
                            show_launcher();
                        }
                    } else if (key == XK_Alt_L) {
                        alt_pressed = 1;
                        printf("Alt pressed (state: Alt=%d, Ctrl=%d)\n", alt_pressed, ctrl_pressed);
                        if (alt_pressed && ctrl_pressed) {
                            printf("Alt+Ctrl detected — toggling launcher\n");
                            fflush(stdout);
                            if (launcher.visible) {
                                hide_launcher();
                            } else {
                                show_launcher();
                            }
                            // Reset states to prevent repeated triggers
                            alt_pressed = 0;
                            ctrl_pressed = 0;
                        }
                    } else if (key == XK_Control_L) {
                        ctrl_pressed = 1;
                        printf("Ctrl pressed (state: Alt=%d, Ctrl=%d)\n", alt_pressed, ctrl_pressed);
                        if (alt_pressed && ctrl_pressed) {
                            printf("Alt+Ctrl detected — toggling launcher\n");
                            fflush(stdout);
                            if (launcher.visible) {
                                hide_launcher();
                            } else {
                                show_launcher();
                            }
                            // Reset states to prevent repeated triggers
                            alt_pressed = 0;
                            ctrl_pressed = 0;
                        }
                    } else if ((ev.xkey.state & Mod1Mask) && (ev.xkey.state & ControlMask) && key == XK_l) {
                        // Alt+Ctrl+L combination for launcher (backup method)
                        printf("Alt+Ctrl+L pressed — toggling launcher\n");
                        fflush(stdout);
                        if (launcher.visible) {
                            hide_launcher();
                        } else {
                            show_launcher();
                        }
                    } else if (launcher.visible && key == XK_Escape) {
                        // Close launcher with Escape key
                        hide_launcher();
                    } else {
                        // Debug: print any other key press to see what we're getting
                        printf("Key pressed: %s (0x%lx), state: 0x%x\n", 
                               XKeysymToString(key), key, ev.xkey.state);
                        fflush(stdout);
                    }
                }
                break;

            case KeyRelease:
                {
                    KeySym key = XLookupKeysym(&ev.xkey, 0);
                    if (key == XK_Alt_L) {
                        alt_pressed = 0;
                        printf("Alt released\n");
                    } else if (key == XK_Control_L) {
                        ctrl_pressed = 0;
                        printf("Ctrl released\n");
                    }
                }
                break;

            case ButtonPress:
                if (launcher.visible) {
                    handle_launcher_click(&ev);
                } else {
                    handle_button_press(&ev);
                }
                break;

            case MotionNotify:
                handle_motion_notify(&ev);
                break;

            case ButtonRelease:
                handle_button_release(&ev);
                break;

            case Expose:
                if (launcher.visible) {
                    handle_launcher_expose(&ev);
                }
                handle_expose(&ev);
                break;

            case DestroyNotify:
                handle_destroy_notify(&ev);
                break;

            case UnmapNotify:
                handle_unmap_notify(&ev);
                break;
        }
    }
    return 0;
}
