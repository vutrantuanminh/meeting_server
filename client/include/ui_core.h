#ifndef UI_CORE_H
#define UI_CORE_H

#include <ncurses.h>
#include <stdbool.h>

// Color pairs
#define COLOR_HEADER    1
#define COLOR_SUCCESS   2
#define COLOR_ERROR     3
#define COLOR_HIGHLIGHT 4
#define COLOR_BORDER    5
#define COLOR_INFO      6

// UI initialization
void init_ui();
void cleanup_ui();

// Window management
WINDOW* create_window(int h, int w, int y, int x);
void destroy_window(WINDOW* win);

// Message display
void show_message(const char* msg, int color);
void show_error(const char* msg);
void show_success(const char* msg);
void show_info(const char* msg);

// Utility
void clear_screen();
void draw_header(const char* title);

#endif
