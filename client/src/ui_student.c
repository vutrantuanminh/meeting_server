#include "ui_student.h"
#include "network.h"
#include "protocol_client.h"
#include "ui_components.h"
#include "ui_core.h"
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

static void view_free_slots(int sockfd, const char *token);
static void view_my_meetings(int sockfd, const char *token);
static void do_cancel_meeting(int sockfd, const char *token,
                              const char *meeting_id);
static void do_view_minutes(int sockfd, const char *token,
                            const char *meeting_id);

void show_student_menu(int sockfd, const char *token, const char *username) {
  char title[128];
  snprintf(title, sizeof(title), "STUDENT MENU - Welcome, %s", username);

  const char *items[] = {"View Free Slots", "My Meetings", "Logout"};

  while (1) {
    clear_screen();
    int choice = show_menu(title, items, 3);

    if (choice == -1 || choice == 2) { // ESC or Logout
      break;
    }

    switch (choice) {
    case 0:
      view_free_slots(sockfd, token);
      break;
    case 1:
      view_my_meetings(sockfd, token);
      break;
    }
  }
}

static void view_free_slots(int sockfd, const char *token) {
  clear_screen();
  draw_header("FREE SLOTS");

  show_info("Loading free slots...");

  // Send LIST_FREE_SLOTS request
  if (send_request(sockfd, "LIST_FREE_SLOTS", token, "") < 0) {
    show_error("Failed to send request");
    napms(2000);
    return;
  }

  char *raw_response = receive_response(sockfd);
  if (!raw_response) {
    show_error("No response from server");
    napms(2000);
    return;
  }

  Response *res = parse_response(raw_response);
  if (!res || res->status_code != STATUS_OK) {
    show_error(res ? res->payload : "Invalid response");
    if (res)
      free_response(res);
    napms(2000);
    return;
  }

  // Parse slots
  int field_count;
  char **fields = parse_payload_fields(res->payload, &field_count);

  if (field_count == 0 ||
      (field_count == 1 && strcmp(fields[0], "EMPTY") == 0)) {
    show_info("No free slots available");
    free_fields(fields, field_count);
    free_response(res);
    napms(2000);
    return;
  }

  // Display in table format
  clear_screen();
  draw_header("FREE SLOTS - Enter SlotID to Book");

  int y = 3;
  attron(COLOR_PAIR(COLOR_HEADER) | A_BOLD);
  mvprintw(y++, 2, "%-6s %-10s %-10s %-11s %-6s %-6s %-10s", "SlotID",
           "TeacherID", "Teacher", "Date", "Start", "End", "Type");
  attroff(COLOR_PAIR(COLOR_HEADER) | A_BOLD);
  mvhline(y++, 2, ACS_HLINE, 70);

  for (int i = 0; i < field_count && y < LINES - 8; i++) {
    char *slot_copy = strdup(fields[i]);
    char *parts[6];
    int part_idx = 0;
    char *token_ptr = strtok(slot_copy, "&");
    while (token_ptr && part_idx < 6) {
      parts[part_idx++] = token_ptr;
      token_ptr = strtok(NULL, "&");
    }

    if (part_idx >= 6) {
      char date[20], time1[20], time2[20];
      sscanf(parts[3], "%s %s", date, time1); // Get date and start time
      sscanf(parts[4], "%*s %s", time2);      // Get end time only

      mvprintw(y++, 2, "%-6s %-10s %-10s %-11s %-6s %-6s %-10s", parts[0],
               parts[1], parts[2], date, time1, time2, parts[5]);
    }
    free(slot_copy);
  }

  free_fields(fields, field_count);
  free_response(res);

  // Prompt for SlotID
  mvprintw(y + 1, 2, "Enter SlotID to book (or 0 to cancel): ");
  refresh();

  char slot_input[20];
  memset(slot_input, 0, sizeof(slot_input));
  echo();
  getnstr(slot_input, 19);
  noecho();

  if (strlen(slot_input) == 0 || strcmp(slot_input, "0") == 0) {
    return;
  }

  // Book the slot - no need to ask for TeacherID
  clear_screen();
  draw_header("BOOK MEETING");

  const char *type_items[] = {"Individual Meeting", "Group Meeting", "Cancel"};
  int book_type = show_menu("Select Meeting Type", type_items, 3);

  if (book_type == -1 || book_type == 2)
    return;

  if (book_type == 0) {
    // Book individual - only slot_id
    show_info("Booking individual meeting...");
    if (send_request(sockfd, "BOOK_INDIVIDUAL", token, slot_input) < 0) {
      show_error("Failed to send request");
    } else {
      char *raw = receive_response(sockfd);
      Response *r = parse_response(raw);
      if (r && r->status_code == STATUS_OK) {
        show_success("Meeting booked successfully!");
      } else {
        show_error(r ? r->payload : "Booking failed");
      }
      if (r)
        free_response(r);
    }
    napms(2000);
  } else {
    // Book group - slot_id&member_ids
    char *members = show_input_form("Member IDs (comma separated):", false);
    if (!members)
      return;

    for (char *p = members; *p; p++) {
      if (*p == ',')
        *p = '|';
    }

    char data[512];
    snprintf(data, sizeof(data), "%s&%s", slot_input, members);

    show_info("Booking group meeting...");
    if (send_request(sockfd, "BOOK_GROUP", token, data) < 0) {
      show_error("Failed to send request");
    } else {
      char *raw = receive_response(sockfd);
      Response *r = parse_response(raw);
      if (r && r->status_code == STATUS_OK) {
        show_success("Group meeting booked successfully!");
      } else {
        show_error(r ? r->payload : "Booking failed");
      }
      if (r)
        free_response(r);
    }
    free(members);
    napms(2000);
  }
}

static void view_my_meetings(int sockfd, const char *token) {
  while (1) {
    clear_screen();
    draw_header("MY MEETINGS");

    const char *filter_items[] = {"Today", "This Week", "All", "Back"};
    int filter = show_menu("Select Filter", filter_items, 4);

    if (filter == -1 || filter == 3) // ESC or Back
      return;

    const char *filter_data[] = {"date", "week", ""};

    show_info("Loading meetings...");

    if (send_request(sockfd, "LIST_MEETINGS", token, filter_data[filter]) < 0) {
      show_error("Failed to send request");
      napms(2000);
      continue;
    }

    char *raw_response = receive_response(sockfd);
    Response *res = parse_response(raw_response);

    if (!res || res->status_code != STATUS_OK) {
      show_error(res ? res->payload : "Failed to load meetings");
      if (res)
        free_response(res);
      napms(2000);
      continue;
    }

    // Parse payload
    int field_count;
    char **fields = parse_payload_fields(res->payload, &field_count);

    if (field_count == 0 ||
        (field_count == 1 && strcmp(fields[0], "EMPTY") == 0)) {
      show_info("No meetings found");
      free_fields(fields, field_count);
      free_response(res);
      napms(2000);
      continue;
    }

    clear_screen();
    draw_header("MY MEETINGS - Enter MeetingID to Manage");

    int y = 3;
    attron(COLOR_PAIR(COLOR_HEADER) | A_BOLD);
    mvprintw(y++, 2, "%-10s %-12s %-8s %-12s %-10s", "MeetingID", "Date",
             "Time", "Teacher", "Type");
    attroff(COLOR_PAIR(COLOR_HEADER) | A_BOLD);
    mvhline(y++, 2, ACS_HLINE, 60);

    for (int i = 0; i < field_count && y < LINES - 8; i++) {
      // Parse: meeting_id&start_time&end_time&teacher&is_group
      char *copy = strdup(fields[i]);
      char *parts[5];
      int idx = 0;
      char *tok = strtok(copy, "&");
      while (tok && idx < 5) {
        parts[idx++] = tok;
        tok = strtok(NULL, "&");
      }

      if (idx >= 5) {
        char date[20], time_str[20];
        sscanf(parts[1], "%s %s", date, time_str);
        const char *type =
            (strcmp(parts[4], "0") == 0) ? "Individual" : "Group";

        mvprintw(y++, 2, "%-10s %-12s %-8s %-12s %-10s", parts[0], date,
                 time_str, parts[3], type);
      }
      free(copy);
    }

    free_fields(fields, field_count);
    free_response(res);

    // Prompt for MeetingID
    mvprintw(y + 1, 2, "Enter MeetingID to manage (or 0 to go back): ");
    refresh();

    char meeting_input[20];
    echo();
    getnstr(meeting_input, 19);
    noecho();

    if (strlen(meeting_input) == 0 || strcmp(meeting_input, "0") == 0) {
      continue;
    }

    // Show sub-menu for the entered meeting ID
    char submenu_title[64];
    snprintf(submenu_title, sizeof(submenu_title), "Meeting ID: %s",
             meeting_input);

    const char *action_items[] = {"View Meeting Minutes", "Cancel Meeting",
                                  "Back"};
    int action = show_menu(submenu_title, action_items, 3);

    switch (action) {
    case 0: // View Minutes
      do_view_minutes(sockfd, token, meeting_input);
      break;
    case 1: // Cancel Meeting
      do_cancel_meeting(sockfd, token, meeting_input);
      break;
    default:
      break;
    }
  }
}

// Helper function to cancel a specific meeting
static void do_cancel_meeting(int sockfd, const char *token,
                              const char *meeting_id) {
  if (!show_confirm("Are you sure you want to cancel this meeting?")) {
    return;
  }

  show_info("Cancelling meeting...");

  if (send_request(sockfd, "CANCEL_MEETING", token, meeting_id) < 0) {
    show_error("Failed to send request");
    napms(2000);
    return;
  }

  char *raw_response = receive_response(sockfd);
  Response *res = parse_response(raw_response);

  if (res && res->status_code == STATUS_OK) {
    show_success("Meeting cancelled successfully!");
  } else {
    show_error(res ? res->payload : "Cancellation failed");
  }

  if (res)
    free_response(res);
  napms(2000);
}

// Helper function to view minutes for a specific meeting
static void do_view_minutes(int sockfd, const char *token,
                            const char *meeting_id) {
  show_info("Loading minutes...");

  if (send_request(sockfd, "GET_MINUTES", token, meeting_id) < 0) {
    show_error("Failed to send request");
    napms(2000);
    return;
  }

  char *raw_response = receive_response(sockfd);
  Response *res = parse_response(raw_response);

  if (!res || res->status_code != STATUS_OK) {
    if (res && strstr(res->payload, "NOT_FOUND")) {
      show_info("No minutes available for this meeting");
    } else {
      show_error(res ? res->payload : "Failed to load minutes");
    }
    if (res)
      free_response(res);
    napms(2000);
    return;
  }

  // Display minutes
  clear_screen();
  draw_header("MEETING MINUTES");

  mvprintw(3, 2, "Meeting ID: %s", meeting_id);
  mvprintw(5, 2, "Content:");
  mvprintw(6, 2, "----------------------------------------");

  // Parse payload: GET_MINUTES_SUCCESS||<content>
  int content_field_count;
  char **content_fields =
      parse_payload_fields(res->payload, &content_field_count);

  // content_fields[0] = "GET_MINUTES_SUCCESS", content_fields[1] = actual
  // content
  if (content_field_count > 1 && content_fields[1] != NULL) {
    int y = 7;
    char *content = strdup(content_fields[1]);

    // Word wrap the content
    char *line = strtok(content, "\n");
    while (line && y < LINES - 5) {
      mvprintw(y++, 2, "%s", line);
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

  free_response(res);
}
