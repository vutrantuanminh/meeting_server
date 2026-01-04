#include "ui_components.h"
#include "ui_core.h"
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

int show_menu(const char *title, const char **items, int count) {
  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);

  int win_height = count + 6;
  int win_width = 60;
  int start_y = (max_y - win_height) / 2;
  int start_x = (max_x - win_width) / 2;

  WINDOW *menu_win = create_window(win_height, win_width, start_y, start_x);

  // Draw title
  wattron(menu_win, COLOR_PAIR(COLOR_HEADER) | A_BOLD);
  mvwprintw(menu_win, 1, (win_width - strlen(title)) / 2, "%s", title);
  wattroff(menu_win, COLOR_PAIR(COLOR_HEADER) | A_BOLD);

  // Draw separator
  mvwhline(menu_win, 2, 1, ACS_HLINE, win_width - 2);

  int selected = 0;
  int ch;

  while (1) {
    // Draw menu items
    for (int i = 0; i < count; i++) {
      if (i == selected) {
        wattron(menu_win, COLOR_PAIR(COLOR_HIGHLIGHT) | A_BOLD);
      }
      mvwprintw(menu_win, 3 + i, 3, "%-*s", win_width - 6, items[i]);
      if (i == selected) {
        wattroff(menu_win, COLOR_PAIR(COLOR_HIGHLIGHT) | A_BOLD);
      }
    }

    wrefresh(menu_win);

    ch = getch();

    switch (ch) {
    case KEY_UP:
      selected = (selected - 1 + count) % count;
      break;
    case KEY_DOWN:
      selected = (selected + 1) % count;
      break;
    case 10: // Enter
      destroy_window(menu_win);
      return selected;
    case 27: // ESC
      destroy_window(menu_win);
      return -1;
    }
  }
}

char *show_input_form(const char *prompt, bool is_password) {
  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);

  int win_height = 7;
  int win_width = 60;
  int start_y = (max_y - win_height) / 2;
  int start_x = (max_x - win_width) / 2;

  WINDOW *input_win = create_window(win_height, win_width, start_y, start_x);

  // Draw prompt
  wattron(input_win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
  mvwprintw(input_win, 1, 2, "%s", prompt);
  wattroff(input_win, COLOR_PAIR(COLOR_INFO) | A_BOLD);

  // Input field
  mvwprintw(input_win, 3, 2, "> ");
  wrefresh(input_win);

  // Enable echo temporarily
  if (!is_password) {
    echo();
  }
  curs_set(1);

  char *input = malloc(256);
  memset(input, 0, 256);

  int pos = 0;
  int ch;

  while (1) {
    ch = wgetch(input_win);

    if (ch == 10 || ch == KEY_ENTER) { // Enter
      break;
    } else if (ch == 27) { // ESC
      free(input);
      curs_set(0);
      noecho();
      destroy_window(input_win);
      return NULL;
    } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
      if (pos > 0) {
        pos--;
        input[pos] = '\0';
        mvwprintw(input_win, 3, 4 + pos, " ");
        wmove(input_win, 3, 4 + pos);
      }
    } else if (ch >= 32 && ch < 127 && pos < 255) {
      input[pos++] = ch;
      if (is_password) {
        mvwaddch(input_win, 3, 4 + pos - 1, '*');
      } else {
        mvwaddch(input_win, 3, 4 + pos - 1, ch);
      }
    }

    wrefresh(input_win);
  }

  input[pos] = '\0';

  curs_set(0);
  noecho();
  destroy_window(input_win);

  return input;
}

void show_table(const char *title, const char **headers, char ***rows,
                int row_count, int col_count) {
  clear_screen();
  draw_header(title);

  int y = 3;

  // Draw headers
  attron(COLOR_PAIR(COLOR_HEADER) | A_BOLD);
  move(y, 2);
  for (int i = 0; i < col_count; i++) {
    printw("%-20s ", headers[i]);
  }
  attroff(COLOR_PAIR(COLOR_HEADER) | A_BOLD);
  y++;

  // Draw separator
  mvhline(y++, 2, ACS_HLINE, col_count * 21);

  // Draw rows
  for (int i = 0; i < row_count; i++) {
    move(y++, 2);
    for (int j = 0; j < col_count; j++) {
      printw("%-20s ", rows[i][j]);
    }
  }

  mvprintw(y + 2, 2, "Press any key to continue...");
  refresh();
  getch();
}

bool show_confirm(const char *message) {
  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);

  int win_height = 7;
  int win_width = 50;
  int start_y = (max_y - win_height) / 2;
  int start_x = (max_x - win_width) / 2;

  WINDOW *confirm_win = create_window(win_height, win_width, start_y, start_x);

  wattron(confirm_win, COLOR_PAIR(COLOR_INFO) | A_BOLD);
  mvwprintw(confirm_win, 2, 2, "%s", message);
  wattroff(confirm_win, COLOR_PAIR(COLOR_INFO) | A_BOLD);

  mvwprintw(confirm_win, 4, 2, "[Y]es  [N]o");
  wrefresh(confirm_win);

  int ch;
  bool result = false;

  while (1) {
    ch = wgetch(confirm_win);
    if (ch == 'y' || ch == 'Y') {
      result = true;
      break;
    } else if (ch == 'n' || ch == 'N' || ch == 27) {
      result = false;
      break;
    }
  }

  destroy_window(confirm_win);
  return result;
}

char *show_text_editor(const char *title, const char *initial_text) {
  clear_screen();
  draw_header(title);

  mvprintw(2, 2, "Enter text (Ctrl+D to finish, ESC to cancel):");
  refresh();

  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);

  WINDOW *edit_win = create_window(max_y - 8, max_x - 4, 4, 2);

  echo();
  curs_set(1);

  char *buffer = malloc(4096);
  memset(buffer, 0, 4096);

  if (initial_text) {
    strncpy(buffer, initial_text, 4095);
    mvwprintw(edit_win, 1, 1, "%s", buffer);
  }

  wmove(edit_win, 1, 1);
  wrefresh(edit_win);

  // Simple text input
  int pos = initial_text ? strlen(initial_text) : 0;
  int ch;

  while (1) {
    ch = wgetch(edit_win);

    if (ch == 4) { // Ctrl+D
      break;
    } else if (ch == 27) { // ESC
      free(buffer);
      curs_set(0);
      noecho();
      destroy_window(edit_win);
      return NULL;
    } else if ((ch == KEY_BACKSPACE || ch == 127 || ch == 8) && pos > 0) {
      pos--;
      buffer[pos] = '\0';
    } else if (ch >= 32 && ch < 127 && pos < 4095) {
      buffer[pos++] = ch;
    } else if (ch == 10) { // Enter
      buffer[pos++] = '\n';
    }

    wclear(edit_win);
    box(edit_win, 0, 0);
    mvwprintw(edit_win, 1, 1, "%s", buffer);
    wrefresh(edit_win);
  }

  buffer[pos] = '\0';

  curs_set(0);
  noecho();
  destroy_window(edit_win);

  return buffer;
}
