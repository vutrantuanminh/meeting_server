#ifndef UI_COMPONENTS_H
#define UI_COMPONENTS_H

#include <stdbool.h>

// Menu component
int show_menu(const char* title, const char** items, int count);

// Input forms
char* show_input_form(const char* prompt, bool is_password);

// Table display
void show_table(const char* title, const char** headers, char*** rows, int row_count, int col_count);

// Confirmation dialog
bool show_confirm(const char* message);

// Multi-line text input
char* show_text_editor(const char* title, const char* initial_text);

#endif
