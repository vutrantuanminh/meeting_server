#include "ui_teacher.h"
#include "network.h"
#include "protocol_client.h"
#include "ui_components.h"
#include "ui_core.h"
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

static void manage_slots(int sockfd, const char *token);
static void view_appointments(int sockfd, const char *token);
static void view_student_history(int sockfd, const char *token);

void show_teacher_menu(int sockfd, const char *token, const char *username) {
  char title[128];
  snprintf(title, sizeof(title), "TEACHER MENU - Welcome, %s", username);

  // Merged: View Appointments + Add Minutes -> View Appointments (with minutes)
  const char *items[] = {"Manage Slots", "View Appointments",
                         "View Student History", "Logout"};

  while (1) {
    clear_screen();
    int choice = show_menu(title, items, 4);

    if (choice == -1 || choice == 3) { // ESC or Logout
      send_request(sockfd, "LOGOUT", token, "");
      receive_response(sockfd); // Ignore response
      break;
    }

    switch (choice) {
    case 0:
      manage_slots(sockfd, token);
      break;
    case 1:
      view_appointments(sockfd, token);
      break;
    case 2:
      view_student_history(sockfd, token);
      break;
    }
  }
}

// ============= MANAGE SLOTS - Shows list of slots first =============
static void manage_slots(int sockfd, const char *token) {
  while (1) {
    // First, fetch and display current slots
    clear_screen();
    draw_header("MY SLOTS");

    show_info("Loading slots...");

    if (send_request(sockfd, "LIST_MY_SLOTS", token, "") < 0) {
      show_error("Failed to send request");
      napms(2000);
      return;
    }

    char *raw_response = receive_response(sockfd);
    Response *res = parse_response(raw_response);

    if (!res || res->status_code != STATUS_OK) {
      show_error(res ? res->payload : "Failed to load slots");
      if (res)
        free_response(res);
      napms(2000);
      return;
    }

    // Parse and display slots
    int field_count;
    char **fields = parse_payload_fields(res->payload, &field_count);

    clear_screen();
    draw_header("MY SLOTS");

    int y = 3;
    attron(COLOR_PAIR(COLOR_HEADER) | A_BOLD);
    mvprintw(y++, 2, "%-8s %-12s %-8s %-8s %-12s %-8s", "SlotID", "Date",
             "Start", "End", "Type", "Booked");
    attroff(COLOR_PAIR(COLOR_HEADER) | A_BOLD);
    mvhline(y++, 2, ACS_HLINE, 65);

    if (field_count == 0 ||
        (field_count == 1 && strcmp(fields[0], "EMPTY") == 0)) {
      mvprintw(y++, 2, "(No slots created yet)");
    } else {
      for (int i = 0; i < field_count && y < LINES - 10; i++) {
        // Parse: slot_id&date&start_time&end_time&type&is_booked
        char *copy = strdup(fields[i]);
        char *parts[6];
        int idx = 0;
        char *tok = strtok(copy, "&");
        while (tok && idx < 6) {
          parts[idx++] = tok;
          tok = strtok(NULL, "&");
        }

        if (idx >= 6) {
          const char *booked = strcmp(parts[5], "1") == 0 ? "Yes" : "No";
          mvprintw(y++, 2, "%-8s %-12s %-8s %-8s %-12s %-8s", parts[0],
                   parts[1], parts[2], parts[3], parts[4], booked);
        }
        free(copy);
      }
    }

    free_fields(fields, field_count);
    free_response(res);

    // Show action menu inline (don't use show_menu as it clears screen)
    mvprintw(y + 2, 2, "Actions: [1] Add  [2] Update  [3] Delete  [0] Back");
    mvprintw(y + 3, 2, "Enter choice: ");
    refresh();

    char input[10];
    echo();
    getnstr(input, 9);
    noecho();

    int action = atoi(input);

    if (action == 0)
      break;

    if (action == 1) { // Add slot
      clear_screen();
      draw_header("ADD NEW SLOT");

      char *date = show_input_form("Date (YYYY-MM-DD):", false);
      if (!date)
        continue;

      char *start_time = show_input_form("Start Time (HH:MM):", false);
      if (!start_time) {
        free(date);
        continue;
      }

      char *end_time = show_input_form("End Time (HH:MM):", false);
      if (!end_time) {
        free(date);
        free(start_time);
        continue;
      }

      const char *type_items[] = {"Individual Only", "Group Only", "Both"};
      int slot_type = show_menu("Select Slot Type", type_items, 3);
      if (slot_type == -1) {
        free(date);
        free(start_time);
        free(end_time);
        continue;
      }

      char data[256];
      snprintf(data, sizeof(data), "%s||%s||%s||%d", date, start_time, end_time,
               slot_type);

      show_info("Adding slot...");

      if (send_request(sockfd, "ADD_SLOT", token, data) < 0) {
        show_error("Failed to send request");
      } else {
        char *raw = receive_response(sockfd);
        Response *r = parse_response(raw);
        if (r && r->status_code == STATUS_OK) {
          show_success("Slot added successfully!");
        } else {
          show_error(r ? r->payload : "Failed to add slot");
        }
        if (r)
          free_response(r);
      }

      free(date);
      free(start_time);
      free(end_time);
      napms(2000);

    } else if (action == 2) { // Update slot
      clear_screen();
      draw_header("UPDATE SLOT");

      char *slot_id = show_input_form("Slot ID to update:", false);
      if (!slot_id)
        continue;

      char *date = show_input_form("New Date (YYYY-MM-DD):", false);
      if (!date) {
        free(slot_id);
        continue;
      }

      char *start_time = show_input_form("New Start Time (HH:MM):", false);
      if (!start_time) {
        free(slot_id);
        free(date);
        continue;
      }

      char *end_time = show_input_form("New End Time (HH:MM):", false);
      if (!end_time) {
        free(slot_id);
        free(date);
        free(start_time);
        continue;
      }

      // Ask for slot type
      clear_screen();
      draw_header("UPDATE SLOT - Select Type");
      const char *type_items[] = {"Individual", "Group", "Both"};
      int slot_type = show_menu("Select Slot Type", type_items, 3);
      if (slot_type == -1) {
        free(slot_id);
        free(date);
        free(start_time);
        free(end_time);
        continue;
      }

      // Format: slot_id&start_datetime&end_datetime&slot_type
      char start_datetime[32], end_datetime[32];
      snprintf(start_datetime, sizeof(start_datetime), "%s %s:00", date,
               start_time);
      snprintf(end_datetime, sizeof(end_datetime), "%s %s:00", date, end_time);

      char data[512];
      snprintf(data, sizeof(data), "%s&%s&%s&%d", slot_id, start_datetime,
               end_datetime, slot_type);

      show_info("Updating slot...");

      if (send_request(sockfd, "UPDATE_SLOT", token, data) < 0) {
        show_error("Failed to send request");
      } else {
        char *raw = receive_response(sockfd);
        Response *r = parse_response(raw);
        if (r && r->status_code == STATUS_OK) {
          show_success("Slot updated successfully!");
        } else {
          show_error(r ? r->payload : "Failed to update slot");
        }
        if (r)
          free_response(r);
      }

      free(slot_id);
      free(date);
      free(start_time);
      free(end_time);
      napms(2000);

    } else if (action == 3) { // Delete slot
      clear_screen();
      draw_header("DELETE SLOT");

      char *slot_id = show_input_form("Slot ID to delete:", false);
      if (!slot_id)
        continue;

      if (!show_confirm("Are you sure you want to delete this slot?")) {
        free(slot_id);
        continue;
      }

      show_info("Deleting slot...");

      if (send_request(sockfd, "DELETE_SLOT", token, slot_id) < 0) {
        show_error("Failed to send request");
      } else {
        char *raw = receive_response(sockfd);
        Response *r = parse_response(raw);
        if (r && r->status_code == STATUS_OK) {
          show_success("Slot deleted successfully!");
        } else {
          show_error(r ? r->payload : "Failed to delete slot");
        }
        if (r)
          free_response(r);
      }

      free(slot_id);
      napms(2000);
    }
  }
}

// ============= VIEW APPOINTMENTS - Shows appointments, allows adding minutes
// =============
static void view_appointments(int sockfd, const char *token) {
  while (1) {
    clear_screen();
    draw_header("MY APPOINTMENTS");

    const char *filter_items[] = {"Today", "This Week", "All", "Back"};
    int filter = show_menu("Select Filter", filter_items, 4);

    if (filter == -1 || filter == 3)
      return;

    const char *filter_data[] = {"date", "week", ""};

    show_info("Loading appointments...");

    if (send_request(sockfd, "LIST_APPOINTMENTS", token, filter_data[filter]) <
        0) {
      show_error("Failed to send request");
      napms(2000);
      continue;
    }

    char *raw_response = receive_response(sockfd);
    Response *res = parse_response(raw_response);

    if (!res || res->status_code != STATUS_OK) {
      show_error(res ? res->payload : "Failed to load appointments");
      if (res)
        free_response(res);
      napms(2000);
      continue;
    }

    // Parse and display appointments
    int field_count;
    char **fields = parse_payload_fields(res->payload, &field_count);

    if (field_count == 0 ||
        (field_count == 1 && strcmp(fields[0], "EMPTY") == 0)) {
      show_info("No appointments found");
      free_fields(fields, field_count);
      free_response(res);
      napms(2000);
      continue;
    }

    clear_screen();
    draw_header("MY APPOINTMENTS - Enter MeetingID to Add Minutes");

    int y = 3;
    attron(COLOR_PAIR(COLOR_HEADER) | A_BOLD);
    mvprintw(y++, 2, "%-10s %-12s %-8s %-15s %-10s", "MeetingID", "Date",
             "Time", "Student", "Type");
    attroff(COLOR_PAIR(COLOR_HEADER) | A_BOLD);
    mvhline(y++, 2, ACS_HLINE, 60);

    for (int i = 0; i < field_count && y < LINES - 8; i++) {
      // Parse: meeting_id&start_time&end_time&student&is_group
      char *copy = strdup(fields[i]);
      char *parts[5];
      int idx = 0;
      char *tok = strtok(copy, "&");
      while (tok && idx < 5) {
        parts[idx++] = tok;
        tok = strtok(NULL, "&");
      }

      if (idx >= 5) {
        char date[20], time[20];
        sscanf(parts[1], "%s %s", date, time);
        const char *type =
            (strcmp(parts[4], "0") == 0) ? "Individual" : "Group";

        mvprintw(y++, 2, "%-10s %-12s %-8s %-15s %-10s", parts[0], date, time,
                 parts[3], type);
      }
      free(copy);
    }

    free_fields(fields, field_count);
    free_response(res);

    // Prompt for meeting ID to add/edit minutes
    mvprintw(y + 1, 2, "Enter MeetingID to add/edit minutes (0 to go back): ");
    refresh();

    char meeting_input[20];
    echo();
    getnstr(meeting_input, 19);
    noecho();

    if (strlen(meeting_input) == 0 || strcmp(meeting_input, "0") == 0) {
      continue;
    }

    // First, try to fetch existing minutes for this meeting
    char existing_content[4096] = "";
    int has_existing = 0;

    show_info("Loading existing minutes...");
    if (send_request(sockfd, "GET_MINUTES", token, meeting_input) >= 0) {
      char *raw = receive_response(sockfd);
      Response *r = parse_response(raw);

      if (r && r->status_code == STATUS_OK) {
        // Parse existing content
        int fc;
        char **flds = parse_payload_fields(r->payload, &fc);
        if (fc > 1 && flds[1] != NULL) {
          strncpy(existing_content, flds[1], sizeof(existing_content) - 1);
          has_existing = 1;
        }
        free_fields(flds, fc);
      }
      if (r)
        free_response(r);
    }

    // Open text editor with existing content (or empty if none)
    const char *editor_title =
        has_existing ? "EDIT MEETING MINUTES" : "ADD MEETING MINUTES";
    char *content = show_text_editor(editor_title, existing_content);
    if (!content)
      continue;

    char data[4096];
    snprintf(data, sizeof(data), "%s||%s", meeting_input, content);

    show_info("Saving minutes...");

    if (send_request(sockfd, "ADD_MINUTES", token, data) < 0) {
      show_error("Failed to send request");
    } else {
      char *raw = receive_response(sockfd);
      Response *r = parse_response(raw);
      if (r && r->status_code == STATUS_OK) {
        const char *msg = has_existing ? "Minutes updated successfully!"
                                       : "Minutes added successfully!";
        show_success(msg);
      } else {
        show_error(r ? r->payload : "Failed to save minutes");
      }
      if (r)
        free_response(r);
    }

    free(content);
    napms(2000);
  }
}

// ============= VIEW STUDENT HISTORY - Shows list of students first
// =============
static void view_student_history(int sockfd, const char *token) {
  // First, fetch and display list of students
  clear_screen();
  draw_header("VIEW STUDENT HISTORY");

  show_info("Loading students...");

  if (send_request(sockfd, "LIST_STUDENTS", token, "") < 0) {
    show_error("Failed to send request");
    napms(2000);
    return;
  }

  char *raw_response = receive_response(sockfd);
  Response *res = parse_response(raw_response);

  if (!res || res->status_code != STATUS_OK) {
    show_error(res ? res->payload : "Failed to load students");
    if (res)
      free_response(res);
    napms(2000);
    return;
  }

  // Parse students
  int field_count;
  char **fields = parse_payload_fields(res->payload, &field_count);

  if (field_count == 0 ||
      (field_count == 1 && strcmp(fields[0], "EMPTY") == 0)) {
    show_info("No students found");
    free_fields(fields, field_count);
    free_response(res);
    napms(2000);
    return;
  }

  clear_screen();
  draw_header("SELECT STUDENT");

  int y = 3;
  attron(COLOR_PAIR(COLOR_HEADER) | A_BOLD);
  mvprintw(y++, 2, "%-10s %-20s", "StudentID", "Username");
  attroff(COLOR_PAIR(COLOR_HEADER) | A_BOLD);
  mvhline(y++, 2, ACS_HLINE, 35);

  for (int i = 0; i < field_count && y < LINES - 8; i++) {
    // Parse: user_id&username
    char *copy = strdup(fields[i]);
    char *parts[2];
    int idx = 0;
    char *tok = strtok(copy, "&");
    while (tok && idx < 2) {
      parts[idx++] = tok;
      tok = strtok(NULL, "&");
    }

    if (idx >= 2) {
      mvprintw(y++, 2, "%-10s %-20s", parts[0], parts[1]);
    }
    free(copy);
  }

  free_fields(fields, field_count);
  free_response(res);

  // Prompt for student ID
  mvprintw(y + 1, 2, "Enter StudentID to view history: ");
  refresh();

  char student_input[20];
  echo();
  getnstr(student_input, 19);
  noecho();

  if (strlen(student_input) == 0) {
    return;
  }

  // Fetch history for selected student
  show_info("Loading history...");

  if (send_request(sockfd, "VIEW_HISTORY", token, student_input) < 0) {
    show_error("Failed to send request");
    napms(2000);
    return;
  }

  raw_response = receive_response(sockfd);
  res = parse_response(raw_response);

  if (!res || res->status_code != STATUS_OK) {
    show_error(res ? res->payload : "Failed to load history");
    if (res)
      free_response(res);
    napms(2000);
    return;
  }

  // Parse and display history
  char **history_fields = parse_payload_fields(res->payload, &field_count);

  if (field_count == 0 ||
      (field_count == 1 && strcmp(history_fields[0], "EMPTY") == 0)) {
    show_info("No history found for this student");
    free_fields(history_fields, field_count);
    free_response(res);
    napms(2000);
    return;
  }

  while (1) {
    clear_screen();
    char header[64];
    snprintf(header, sizeof(header), "HISTORY FOR STUDENT %s", student_input);
    draw_header(header);

    int y = 3;
    attron(COLOR_PAIR(COLOR_HEADER) | A_BOLD);
    mvprintw(y++, 2, "%-10s %-20s %-12s", "MeetingID", "Date", "Has Minutes");
    attroff(COLOR_PAIR(COLOR_HEADER) | A_BOLD);
    mvhline(y++, 2, ACS_HLINE, 45);

    for (int i = 0; i < field_count && y < LINES - 8; i++) {
      // Parse: meeting_id&datetime&has_minutes
      char *copy = strdup(history_fields[i]);
      char *parts[3];
      int idx = 0;
      char *tok = strtok(copy, "&");
      while (tok && idx < 3) {
        parts[idx++] = tok;
        tok = strtok(NULL, "&");
      }

      if (idx >= 3) {
        char date[20];
        sscanf(parts[1], "%s", date);
        const char *has_min = (strcmp(parts[2], "1") == 0) ? "Yes" : "No";

        mvprintw(y++, 2, "%-10s %-20s %-12s", parts[0], date, has_min);
      }
      free(copy);
    }

    mvprintw(y + 1, 2, "Enter MeetingID to view minutes (0 to go back): ");
    refresh();

    char meeting_input[20];
    echo();
    getnstr(meeting_input, 19);
    noecho();

    if (strlen(meeting_input) == 0 || strcmp(meeting_input, "0") == 0)
      break;

    // Request minutes
    if (send_request(sockfd, "GET_MINUTES", token, meeting_input) >= 0) {
      char *raw = receive_response(sockfd);
      Response *r = parse_response(raw);

      if (r && r->status_code == STATUS_OK) {
        clear_screen();
        draw_header("MEETING MINUTES");
        mvprintw(3, 2, "Meeting ID: %s", meeting_input);
        mvprintw(5, 2, "Content:");
        mvprintw(6, 2, "----------------------------------------");

        int content_field_count;
        char **content_fields =
            parse_payload_fields(r->payload, &content_field_count);

        // content_fields[0] = "GET_MINUTES_SUCCESS", content_fields[1] = actual
        // content
        if (content_field_count > 1 && content_fields[1] != NULL) {
          int content_y = 7;
          char *content = strdup(content_fields[1]);

          char *line = strtok(content, "\n");
          while (line && content_y < LINES - 5) {
            mvprintw(content_y++, 2, "%s", line);
            line = strtok(NULL, "\n");
          }

          free(content);
          free_fields(content_fields, content_field_count);
        } else {
          free_fields(content_fields, content_field_count);
          mvprintw(7, 2, "No content available");
        }

        mvprintw(LINES - 3, 2, "Press any key to continue...");
        refresh();
        getch();
      } else {
        show_error(r ? r->payload : "No minutes found");
        napms(2000);
      }
      if (r)
        free_response(r);
    }
  }

  free_fields(history_fields, field_count);
  free_response(res);
}
