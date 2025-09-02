#include "mywm.h"

void init_launcher(void) {
    launcher.visible = 0;
    launcher.app_count = 0;
    
    // Create launcher window (initially unmapped)
    launcher.window = XCreateSimpleWindow(display, root,
        (DisplayWidth(display, DefaultScreen(display)) - LAUNCHER_WIDTH) / 2,
        (DisplayHeight(display, DefaultScreen(display)) - LAUNCHER_HEIGHT) / 2,
        LAUNCHER_WIDTH, LAUNCHER_HEIGHT,
        2,
        BlackPixel(display, DefaultScreen(display)),
        WhitePixel(display, DefaultScreen(display))
    );
    
    // Select input events for the launcher
    XSelectInput(display, launcher.window, 
        ExposureMask | ButtonPressMask | KeyPressMask | StructureNotifyMask);
    
    // Allocate memory for app buttons
    launcher.app_buttons = malloc(MAX_APPS * sizeof(Window));
    
    // Scan for applications
    scan_applications();
    
    // Create buttons for each app
    for (int i = 0; i < launcher.app_count; i++) {
        int row = i / APP_GRID_COLS;
        int col = i % APP_GRID_COLS;
        
        int x = col * (APP_ICON_SIZE + APP_GRID_SPACING) + APP_GRID_SPACING;
        int y = row * (APP_ICON_SIZE + APP_GRID_SPACING * 2) + APP_GRID_SPACING;
        
        launcher.apps[i].x = x;
        launcher.apps[i].y = y;
        
        launcher.app_buttons[i] = XCreateSimpleWindow(display, launcher.window,
            x, y, APP_ICON_SIZE, APP_ICON_SIZE + 20,  // Extra height for text
            1,
            BlackPixel(display, DefaultScreen(display)),
            0xE0E0E0  // Light gray
        );
        
        XSelectInput(display, launcher.app_buttons[i], 
            ButtonPressMask | ExposureMask | EnterWindowMask | LeaveWindowMask);
    }
}

void show_launcher(void) {
    if (launcher.visible) return;
    
    launcher.visible = 1;
    
    // Map the launcher window
    XMapWindow(display, launcher.window);
    
    // Map all app buttons
    for (int i = 0; i < launcher.app_count; i++) {
        XMapWindow(display, launcher.app_buttons[i]);
    }
    
    // Raise the launcher to the top
    XRaiseWindow(display, launcher.window);
    
    // Set focus to launcher for keyboard events
    XSetInputFocus(display, launcher.window, RevertToParent, CurrentTime);
}

void hide_launcher(void) {
    if (!launcher.visible) return;
    
    launcher.visible = 0;
    
    // Unmap the launcher window (this also unmaps children)
    XUnmapWindow(display, launcher.window);
}

void scan_applications(void) {
    launcher.app_count = 0;
    
    // Common directories for .desktop files
    const char *desktop_dirs[] = {
        "/usr/share/applications",
        "/usr/local/share/applications",
        NULL
    };
    
    // Also check user's local applications directory
    char user_apps_dir[512];
    const char *home = getenv("HOME");
    if (home) {
        snprintf(user_apps_dir, sizeof(user_apps_dir), "%s/.local/share/applications", home);
    }
    
    for (int dir_idx = 0; desktop_dirs[dir_idx] != NULL; dir_idx++) {
        DIR *dir = opendir(desktop_dirs[dir_idx]);
        if (!dir) continue;
        
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL && launcher.app_count < MAX_APPS) {
            if (strstr(entry->d_name, ".desktop") == NULL) continue;
            
            char filepath[1024];
            snprintf(filepath, sizeof(filepath), "%s/%s", desktop_dirs[dir_idx], entry->d_name);
            
            FILE *file = fopen(filepath, "r");
            if (!file) continue;
            
            char line[512];
            char name[256] = "";
            char exec[512] = "";
            char icon[512] = "";
            int in_desktop_entry = 0;
            int hidden = 0;
            
            while (fgets(line, sizeof(line), file)) {
                // Remove newline
                line[strcspn(line, "\n")] = 0;
                
                if (strcmp(line, "[Desktop Entry]") == 0) {
                    in_desktop_entry = 1;
                    continue;
                }
                
                if (line[0] == '[' && in_desktop_entry) {
                    break;  // End of Desktop Entry section
                }
                
                if (!in_desktop_entry) continue;
                
                if (strncmp(line, "Name=", 5) == 0 && strlen(name) == 0) {
                    strncpy(name, line + 5, sizeof(name) - 1);
                    name[sizeof(name) - 1] = '\0';  // Ensure null termination
                } else if (strncmp(line, "Exec=", 5) == 0) {
                    strncpy(exec, line + 5, sizeof(exec) - 1);
                    exec[sizeof(exec) - 1] = '\0';  // Ensure null termination
                } else if (strncmp(line, "Icon=", 5) == 0) {
                    strncpy(icon, line + 5, sizeof(icon) - 1);
                    icon[sizeof(icon) - 1] = '\0';  // Ensure null termination
                } else if (strncmp(line, "Hidden=true", 11) == 0 || 
                          strncmp(line, "NoDisplay=true", 14) == 0) {
                    hidden = 1;
                }
            }
            
            fclose(file);
            
            // Add app if it has required fields and isn't hidden
            if (strlen(name) > 0 && strlen(exec) > 0 && !hidden) {
                strncpy(launcher.apps[launcher.app_count].name, name, sizeof(launcher.apps[launcher.app_count].name) - 1);
                launcher.apps[launcher.app_count].name[sizeof(launcher.apps[launcher.app_count].name) - 1] = '\0';
                strncpy(launcher.apps[launcher.app_count].exec, exec, sizeof(launcher.apps[launcher.app_count].exec) - 1);
                launcher.apps[launcher.app_count].exec[sizeof(launcher.apps[launcher.app_count].exec) - 1] = '\0';
                strncpy(launcher.apps[launcher.app_count].icon, icon, sizeof(launcher.apps[launcher.app_count].icon) - 1);
                launcher.apps[launcher.app_count].icon[sizeof(launcher.apps[launcher.app_count].icon) - 1] = '\0';
                launcher.app_count++;
            }
        }
        
        closedir(dir);
    }
    
    // Also scan user's local applications directory
    if (home && launcher.app_count < MAX_APPS) {
        DIR *dir = opendir(user_apps_dir);
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL && launcher.app_count < MAX_APPS) {
                if (strstr(entry->d_name, ".desktop") == NULL) continue;
                
                char filepath[1024];
                snprintf(filepath, sizeof(filepath), "%s/%s", user_apps_dir, entry->d_name);
                
                FILE *file = fopen(filepath, "r");
                if (!file) continue;
                
                char line[512];
                char name[256] = "";
                char exec[512] = "";
                char icon[512] = "";
                int in_desktop_entry = 0;
                int hidden = 0;
                
                while (fgets(line, sizeof(line), file)) {
                    // Remove newline
                    line[strcspn(line, "\n")] = 0;
                    
                    if (strcmp(line, "[Desktop Entry]") == 0) {
                        in_desktop_entry = 1;
                        continue;
                    }
                    
                    if (line[0] == '[' && in_desktop_entry) {
                        break;  // End of Desktop Entry section
                    }
                    
                    if (!in_desktop_entry) continue;
                    
                    if (strncmp(line, "Name=", 5) == 0 && strlen(name) == 0) {
                        strncpy(name, line + 5, sizeof(name) - 1);
                        name[sizeof(name) - 1] = '\0';  // Ensure null termination
                    } else if (strncmp(line, "Exec=", 5) == 0) {
                        strncpy(exec, line + 5, sizeof(exec) - 1);
                        exec[sizeof(exec) - 1] = '\0';  // Ensure null termination
                    } else if (strncmp(line, "Icon=", 5) == 0) {
                        strncpy(icon, line + 5, sizeof(icon) - 1);
                        icon[sizeof(icon) - 1] = '\0';  // Ensure null termination
                    } else if (strncmp(line, "Hidden=true", 11) == 0 || 
                              strncmp(line, "NoDisplay=true", 14) == 0) {
                        hidden = 1;
                    }
                }
                
                fclose(file);
                
                // Add app if it has required fields and isn't hidden
                if (strlen(name) > 0 && strlen(exec) > 0 && !hidden) {
                    strncpy(launcher.apps[launcher.app_count].name, name, sizeof(launcher.apps[launcher.app_count].name) - 1);
                    launcher.apps[launcher.app_count].name[sizeof(launcher.apps[launcher.app_count].name) - 1] = '\0';
                    strncpy(launcher.apps[launcher.app_count].exec, exec, sizeof(launcher.apps[launcher.app_count].exec) - 1);
                    launcher.apps[launcher.app_count].exec[sizeof(launcher.apps[launcher.app_count].exec) - 1] = '\0';
                    strncpy(launcher.apps[launcher.app_count].icon, icon, sizeof(launcher.apps[launcher.app_count].icon) - 1);
                    launcher.apps[launcher.app_count].icon[sizeof(launcher.apps[launcher.app_count].icon) - 1] = '\0';
                    launcher.app_count++;
                }
            }
            
            closedir(dir);
        }
    }
    
    printf("Found %d applications\n", launcher.app_count);
}

void handle_launcher_click(XEvent *ev) {
    if (!launcher.visible) return;
    
    // Check if click was on an app button
    for (int i = 0; i < launcher.app_count; i++) {
        if (ev->xbutton.window == launcher.app_buttons[i]) {
            // Launch the application
            char *exec = launcher.apps[i].exec;
            
            // Remove %U, %F and other field codes from exec
            char clean_exec[512];
            char *src = exec;
            char *dst = clean_exec;
            while (*src && dst - clean_exec < sizeof(clean_exec) - 1) {
                if (*src == '%' && *(src + 1)) {
                    src += 2;  // Skip field codes
                } else {
                    *dst++ = *src++;
                }
            }
            *dst = '\0';
            
            printf("Launching: %s\n", clean_exec);
            
            if (fork() == 0) {
                // Use shell to handle complex command lines
                execlp("sh", "sh", "-c", clean_exec, NULL);
                perror("execlp failed");
                exit(1);
            }
            
            // Hide launcher after launching app
            hide_launcher();
            break;
        }
    }
    
    // If click was outside app buttons but inside launcher, do nothing
    // If we want to close on background click, we'd check that here
}

void handle_launcher_expose(XEvent *ev) {
    if (!launcher.visible) return;
    
    // Handle expose events for app buttons
    for (int i = 0; i < launcher.app_count; i++) {
        if (ev->xexpose.window == launcher.app_buttons[i]) {
            // Draw app name
            GC gc = XCreateGC(display, launcher.app_buttons[i], 0, NULL);
            XSetForeground(display, gc, BlackPixel(display, DefaultScreen(display)));
            
            char *name = launcher.apps[i].name;
            int text_len = strlen(name);
            
            // Truncate long names
            char display_name[20];
            if (text_len > 12) {
                strncpy(display_name, name, 9);
                strcpy(display_name + 9, "...");
            } else {
                strcpy(display_name, name);
            }
            
            // Center text horizontally, position near bottom
            int text_width = strlen(display_name) * 6;  // Approximate
            int x = (APP_ICON_SIZE - text_width) / 2;
            int y = APP_ICON_SIZE + 15;
            
            XDrawString(display, launcher.app_buttons[i], gc, x, y, 
                       display_name, strlen(display_name));
            
            XFreeGC(display, gc);
            break;
        }
    }
    
    // Handle main launcher window expose
    if (ev->xexpose.window == launcher.window) {
        GC gc = XCreateGC(display, launcher.window, 0, NULL);
        XSetForeground(display, gc, BlackPixel(display, DefaultScreen(display)));
        
        // Draw title
        char *title = "Applications";
        int title_x = (LAUNCHER_WIDTH - strlen(title) * 8) / 2;
        XDrawString(display, launcher.window, gc, title_x, 20, title, strlen(title));
        
        XFreeGC(display, gc);
    }
}