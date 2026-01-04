#include "ui_core.h"
#include <stdlib.h>
#include <string.h>

void init_ui() {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  // Initialize colors
  if (has_colors()) {
    start_color();
    init_pair(COLOR_HEADER, COLOR_CYAN, COLOR_BLACK);
    init_pair(COLOR_SUCCESS, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_ERROR, COLOR_RED, COLOR_BLACK);
    init_pair(COLOR_HIGHLIGHT, COLOR_BLACK, COLOR_WHITE);
    init_pair(COLOR_BORDER, COLOR_BLUE, COLOR_BLACK);
    init_pair(COLOR_INFO, COLOR_YELLOW, COLOR_BLACK);
  }

  refresh();
}

void cleanup_ui() { endwin(); }

WINDOW *create_window(int h, int w, int y, int x) {
  WINDOW *win = newwin(h, w, y, x);
  box(win, 0, 0);
  wrefresh(win);
  return win;
}

void destroy_window(WINDOW *win) {
  if (win) {
    wborder(win, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wrefresh(win);
    delwin(win);
  }
}

void show_message(const char *msg, int color) {
  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);

  attron(COLOR_PAIR(color) | A_BOLD);
  mvprintw(max_y - 2, 2, "%-*s", max_x - 4, msg);
  attroff(COLOR_PAIR(color) | A_BOLD);
  refresh();
}

void show_error(const char *msg) { show_message(msg, COLOR_ERROR); }

void show_success(const char *msg) { show_message(msg, COLOR_SUCCESS); }

void show_info(const char *msg) { show_message(msg, COLOR_INFO); }

void clear_screen() {
  clear();
  refresh();
}

void draw_header(const char *title) {
  int max_x;
  getmaxyx(stdscr, max_x, max_x);

  attron(COLOR_PAIR(COLOR_HEADER) | A_BOLD);
  mvprintw(0, 0, "%-*s", max_x, "");
  mvprintw(0, (max_x - strlen(title)) / 2, "%s", title);
  attroff(COLOR_PAIR(COLOR_HEADER) | A_BOLD);

  refresh();
}
