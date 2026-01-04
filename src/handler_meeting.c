#include "handler_meeting.h"
#include "auth.h"
#include "database.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============= BOOK_INDIVIDUAL =============
Response *handle_book_individual(Request *req, MYSQL *db_conn) {
  Response *res = calloc(1, sizeof(Response));

  // Validate token
  TokenData *token_data = validate_token(req->token);
  if (!token_data) {
    res->status_code = STATUS_TOKEN_INVALID;
    strcpy(res->payload, "BOOK_INDIVIDUAL_INVALID_TOKEN");
    return res;
  }

  // Check role = student
  if (strcmp(token_data->role, "student") != 0) {
    res->status_code = STATUS_FORBIDDEN;
    strcpy(res->payload, "BOOK_INDIVIDUAL_FORBIDDEN");
    free_token_data(token_data);
    return res;
  }

  // Parse data: teacher_id&slot_id
  int field_count;
  char **fields = parse_subfields(req->data, &field_count);

  if (field_count != 2) {
    res->status_code = STATUS_BAD_REQUEST;
    strcpy(res->payload, "BOOK_INDIVIDUAL_INVALID_FORMAT");
    free_token_data(token_data);
    free_split(fields, field_count);
    return res;
  }

  int teacher_id = atoi(trim(fields[0]));
  int slot_id = atoi(trim(fields[1]));

  // Check slot exists, not booked, and allows individual
  char query[1024];
  snprintf(query, sizeof(query),
           "SELECT slot_type, is_booked FROM slots "
           "WHERE slot_id=%d AND teacher_id=%d",
           slot_id, teacher_id);

  MYSQL_RES *result = db_query(db_conn, query);

  if (!result || mysql_num_rows(result) == 0) {
    res->status_code = STATUS_NOT_FOUND;
    strcpy(res->payload, "BOOK_INDIVIDUAL_SLOT_NOT_FOUND");
    if (result)
      mysql_free_result(result);
    free_token_data(token_data);
    free_split(fields, field_count);
    return res;
  }

  MYSQL_ROW row = mysql_fetch_row(result);
  int slot_type = atoi(row[0]);
  int is_booked = atoi(row[1]);
  mysql_free_result(result);

  // Check if slot allows individual (type 0 or 2)
  if (slot_type == 1) {
    res->status_code = STATUS_FORBIDDEN;
    strcpy(res->payload, "BOOK_INDIVIDUAL_SLOT_NOT_SUITABLE");
    free_token_data(token_data);
    free_split(fields, field_count);
    return res;
  }

  if (is_booked) {
    res->status_code = STATUS_CONFLICT;
    strcpy(res->payload, "BOOK_INDIVIDUAL_SLOT_NOT_FREE");
    free_token_data(token_data);
    free_split(fields, field_count);
    return res;
  }

  // Create meeting
  snprintf(query, sizeof(query),
           "INSERT INTO meetings (slot_id, student_id, is_group) "
           "VALUES (%d, %d, 0)",
           slot_id, token_data->user_id);

  int affected = db_execute(db_conn, query);

  if (affected <= 0) {
    res->status_code = STATUS_INTERNAL_ERROR;
    strcpy(res->payload, "BOOK_INDIVIDUAL_INTERNAL_ERROR");
    free_token_data(token_data);
    free_split(fields, field_count);
    return res;
  }

  int meeting_id = mysql_insert_id(db_conn);

  // Mark slot as booked
  snprintf(query, sizeof(query),
           "UPDATE slots SET is_booked=1 WHERE slot_id=%d", slot_id);
  db_execute(db_conn, query);

  // Build response: BOOK_INDIVIDUAL_SUCCESS||meeting_id
  res->status_code = STATUS_OK;
  snprintf(res->payload, sizeof(res->payload), "BOOK_INDIVIDUAL_SUCCESS||%d",
           meeting_id);

  log_message("INFO", "Meeting booked: id=%d, student=%d, slot=%d", meeting_id,
              token_data->user_id, slot_id);

  // Cleanup
  free_token_data(token_data);
  free_split(fields, field_count);

  return res;
}

// ============= BOOK_GROUP =============
Response *handle_book_group(Request *req, MYSQL *db_conn) {
  Response *res = calloc(1, sizeof(Response));

  // Validate token
  TokenData *token_data = validate_token(req->token);
  if (!token_data) {
    res->status_code = STATUS_TOKEN_INVALID;
    strcpy(res->payload, "BOOK_GROUP_INVALID_TOKEN");
    return res;
  }

  // Check role = student
  if (strcmp(token_data->role, "student") != 0) {
    res->status_code = STATUS_FORBIDDEN;
    strcpy(res->payload, "BOOK_GROUP_FORBIDDEN");
    free_token_data(token_data);
    return res;
  }

  // Parse data: teacher_id&slot_id&member_id|member_id|...
  int field_count;
  char **fields = parse_subfields(req->data, &field_count);

  if (field_count < 2) {
    res->status_code = STATUS_BAD_REQUEST;
    strcpy(res->payload, "BOOK_GROUP_INVALID_FORMAT");
    free_token_data(token_data);
    free_split(fields, field_count);
    return res;
  }

  int teacher_id = atoi(trim(fields[0]));
  int slot_id = atoi(trim(fields[1]));

  // Parse member IDs (if provided)
  int *member_ids = NULL;
  int member_count = 0;

  if (field_count > 2) {
    int sub_count;
    char **members = split_string(fields[2], "|", &sub_count);
    member_ids = malloc(sizeof(int) * sub_count);

    for (int i = 0; i < sub_count; i++) {
      member_ids[member_count++] = atoi(trim(members[i]));
    }

    free_split(members, sub_count);
  }

  // Check slot exists and allows group
  char query[1024];
  snprintf(query, sizeof(query),
           "SELECT slot_type, is_booked FROM slots "
           "WHERE slot_id=%d AND teacher_id=%d",
           slot_id, teacher_id);

  MYSQL_RES *result = db_query(db_conn, query);

  if (!result || mysql_num_rows(result) == 0) {
    res->status_code = STATUS_NOT_FOUND;
    strcpy(res->payload, "BOOK_GROUP_SLOT_NOT_FOUND");
    if (result)
      mysql_free_result(result);
    free_token_data(token_data);
    free_split(fields, field_count);
    if (member_ids)
      free(member_ids);
    return res;
  }

  MYSQL_ROW row = mysql_fetch_row(result);
  int slot_type = atoi(row[0]);
  int is_booked = atoi(row[1]);
  mysql_free_result(result);

  // Check if slot allows group (type 1 or 2)
  if (slot_type == 0) {
    res->status_code = STATUS_USERNAME_EXISTS; // 4090
    strcpy(res->payload, "BOOK_GROUP_SLOT_NOT_SUITABLE");
    free_token_data(token_data);
    free_split(fields, field_count);
    if (member_ids)
      free(member_ids);
    return res;
  }

  if (is_booked) {
    res->status_code = STATUS_CONFLICT;
    strcpy(res->payload, "BOOK_GROUP_SLOT_NOT_FREE");
    free_token_data(token_data);
    free_split(fields, field_count);
    if (member_ids)
      free(member_ids);
    return res;
  }

  // Create meeting
  snprintf(query, sizeof(query),
           "INSERT INTO meetings (slot_id, student_id, is_group) "
           "VALUES (%d, %d, 1)",
           slot_id, token_data->user_id);

  int affected = db_execute(db_conn, query);

  if (affected <= 0) {
    res->status_code = STATUS_INTERNAL_ERROR;
    strcpy(res->payload, "BOOK_GROUP_INTERNAL_ERROR");
    free_token_data(token_data);
    free_split(fields, field_count);
    if (member_ids)
      free(member_ids);
    return res;
  }

  int meeting_id = mysql_insert_id(db_conn);

  // Insert group members
  for (int i = 0; i < member_count; i++) {
    snprintf(query, sizeof(query),
             "INSERT INTO group_members (meeting_id, student_id) "
             "VALUES (%d, %d)",
             meeting_id, member_ids[i]);
    db_execute(db_conn, query);
  }

  // Mark slot as booked
  snprintf(query, sizeof(query),
           "UPDATE slots SET is_booked=1 WHERE slot_id=%d", slot_id);
  db_execute(db_conn, query);

  // Build response: BOOK_GROUP_SUCCESS||meeting_id
  res->status_code = STATUS_OK;
  snprintf(res->payload, sizeof(res->payload), "BOOK_GROUP_SUCCESS||%d",
           meeting_id);

  log_message("INFO", "Group meeting booked: id=%d, leader=%d, members=%d",
              meeting_id, token_data->user_id, member_count);

  // Cleanup
  free_token_data(token_data);
  free_split(fields, field_count);
  if (member_ids)
    free(member_ids);

  return res;
}

// ============= CANCEL_MEETING =============
Response *handle_cancel_meeting(Request *req, MYSQL *db_conn) {
  Response *res = calloc(1, sizeof(Response));

  // Validate token
  TokenData *token_data = validate_token(req->token);
  if (!token_data) {
    res->status_code = STATUS_TOKEN_INVALID;
    strcpy(res->payload, "CANCEL_MEETING_INVALID_TOKEN");
    return res;
  }

  // Parse data: meeting_id
  int meeting_id = atoi(trim(req->data));

  // Check meeting exists and belongs to student
  char query[512];
  snprintf(query, sizeof(query),
           "SELECT slot_id, student_id FROM meetings "
           "WHERE meeting_id=%d AND status='pending'",
           meeting_id);

  MYSQL_RES *result = db_query(db_conn, query);

  if (!result || mysql_num_rows(result) == 0) {
    res->status_code = STATUS_NOT_FOUND;
    strcpy(res->payload, "CANCEL_MEETING_NOT_FOUND");
    if (result)
      mysql_free_result(result);
    free_token_data(token_data);
    return res;
  }

  MYSQL_ROW row = mysql_fetch_row(result);
  int slot_id = atoi(row[0]);
  int student_id = atoi(row[1]);
  mysql_free_result(result);

  // Check permission
  if (token_data->user_id != student_id) {
    res->status_code = STATUS_FORBIDDEN;
    strcpy(res->payload, "CANCEL_MEETING_FORBIDDEN");
    free_token_data(token_data);
    return res;
  }

  // Update meeting status
  snprintf(query, sizeof(query),
           "UPDATE meetings SET status='cancelled' WHERE meeting_id=%d",
           meeting_id);

  int affected = db_execute(db_conn, query);

  if (affected <= 0) {
    res->status_code = STATUS_INTERNAL_ERROR;
    strcpy(res->payload, "CANCEL_MEETING_INTERNAL_ERROR");
    free_token_data(token_data);
    return res;
  }

  // Mark slot as free
  snprintf(query, sizeof(query),
           "UPDATE slots SET is_booked=0 WHERE slot_id=%d", slot_id);
  db_execute(db_conn, query);

  // Success
  res->status_code = STATUS_OK;
  strcpy(res->payload, "CANCEL_MEETING_SUCCESS");

  log_message("INFO", "Meeting cancelled: id=%d", meeting_id);

  // Cleanup
  free_token_data(token_data);

  return res;
}

// ============= LIST_MEETINGS (Student) =============
Response *handle_list_meetings(Request *req, MYSQL *db_conn) {
  Response *res = calloc(1, sizeof(Response));

  // Validate token
  TokenData *token_data = validate_token(req->token);
  if (!token_data) {
    res->status_code = STATUS_TOKEN_INVALID;
    strcpy(res->payload, "LIST_MEETINGS_INVALID_TOKEN");
    return res;
  }

  // Parse filter: "date" = today, "week" = this week, "" = all
  char *filter = req->data;

  // Build query with filter
  char query[1024];

  if (filter && strcmp(filter, "date") == 0) {
    // Filter: Today only
    snprintf(
        query, sizeof(query),
        "SELECT m.meeting_id, s.start_time, s.end_time, u.username, m.is_group "
        "FROM meetings m "
        "JOIN slots s ON m.slot_id = s.slot_id "
        "JOIN users u ON s.teacher_id = u.user_id "
        "WHERE m.student_id=%d AND m.status='pending' "
        "AND DATE(s.start_time) = CURDATE() "
        "ORDER BY s.start_time",
        token_data->user_id);
  } else if (filter && strcmp(filter, "week") == 0) {
    // Filter: This week
    snprintf(
        query, sizeof(query),
        "SELECT m.meeting_id, s.start_time, s.end_time, u.username, m.is_group "
        "FROM meetings m "
        "JOIN slots s ON m.slot_id = s.slot_id "
        "JOIN users u ON s.teacher_id = u.user_id "
        "WHERE m.student_id=%d AND m.status='pending' "
        "AND YEARWEEK(s.start_time, 1) = YEARWEEK(CURDATE(), 1) "
        "ORDER BY s.start_time",
        token_data->user_id);
  } else {
    // No filter: All meetings
    snprintf(
        query, sizeof(query),
        "SELECT m.meeting_id, s.start_time, s.end_time, u.username, m.is_group "
        "FROM meetings m "
        "JOIN slots s ON m.slot_id = s.slot_id "
        "JOIN users u ON s.teacher_id = u.user_id "
        "WHERE m.student_id=%d AND m.status='pending' "
        "ORDER BY s.start_time",
        token_data->user_id);
  }

  MYSQL_RES *result = db_query(db_conn, query);

  if (!result) {
    res->status_code = STATUS_INTERNAL_ERROR;
    strcpy(res->payload, "LIST_MEETINGS_INTERNAL_ERROR");
    free_token_data(token_data);
    return res;
  }

  // Build response: meeting_id&date&time&teacher&is_group|...
  char payload[4096] = "LIST_MEETINGS_SUCCESS||";
  int first = 1;

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result))) {
    if (!first)
      strcat(payload, "||");

    char meeting_str[512];
    snprintf(meeting_str, sizeof(meeting_str), "%s&%s&%s&%s&%s", row[0], row[1],
             row[2], row[3], row[4]);
    strcat(payload, meeting_str);

    first = 0;
  }

  if (first) {
    strcpy(payload, "LIST_MEETINGS_SUCCESS||EMPTY");
  }

  res->status_code = STATUS_OK;
  strcpy(res->payload, payload);

  log_message("INFO", "Listed meetings for student_id=%d", token_data->user_id);

  // Cleanup
  mysql_free_result(result);
  free_token_data(token_data);

  return res;
}

// ============= LIST_APPOINTMENTS (Teacher) =============
Response *handle_list_appointments(Request *req, MYSQL *db_conn) {
  Response *res = calloc(1, sizeof(Response));

  // Validate token
  TokenData *token_data = validate_token(req->token);
  if (!token_data) {
    res->status_code = STATUS_TOKEN_INVALID;
    strcpy(res->payload, "LIST_APPOINTMENTS_INVALID_TOKEN");
    return res;
  }

  // Check role = teacher
  if (strcmp(token_data->role, "teacher") != 0) {
    res->status_code = STATUS_FORBIDDEN;
    strcpy(res->payload, "LIST_APPOINTMENTS_FORBIDDEN");
    free_token_data(token_data);
    return res;
  }

  // Parse filter: "date" = today, "week" = this week, "" = all
  char *filter = req->data;

  // Build query with filter
  char query[1024];

  if (filter && strcmp(filter, "date") == 0) {
    // Filter: Today only
    snprintf(
        query, sizeof(query),
        "SELECT m.meeting_id, s.start_time, s.end_time, u.username, m.is_group "
        "FROM meetings m "
        "JOIN slots s ON m.slot_id = s.slot_id "
        "JOIN users u ON m.student_id = u.user_id "
        "WHERE s.teacher_id=%d AND m.status='pending' "
        "AND DATE(s.start_time) = CURDATE() "
        "ORDER BY s.start_time",
        token_data->user_id);
  } else if (filter && strcmp(filter, "week") == 0) {
    // Filter: This week
    snprintf(
        query, sizeof(query),
        "SELECT m.meeting_id, s.start_time, s.end_time, u.username, m.is_group "
        "FROM meetings m "
        "JOIN slots s ON m.slot_id = s.slot_id "
        "JOIN users u ON m.student_id = u.user_id "
        "WHERE s.teacher_id=%d AND m.status='pending' "
        "AND YEARWEEK(s.start_time, 1) = YEARWEEK(CURDATE(), 1) "
        "ORDER BY s.start_time",
        token_data->user_id);
  } else {
    // No filter: All appointments
    snprintf(
        query, sizeof(query),
        "SELECT m.meeting_id, s.start_time, s.end_time, u.username, m.is_group "
        "FROM meetings m "
        "JOIN slots s ON m.slot_id = s.slot_id "
        "JOIN users u ON m.student_id = u.user_id "
        "WHERE s.teacher_id=%d AND m.status='pending' "
        "ORDER BY s.start_time",
        token_data->user_id);
  }

  MYSQL_RES *result = db_query(db_conn, query);

  if (!result) {
    res->status_code = STATUS_INTERNAL_ERROR;
    strcpy(res->payload, "LIST_APPOINTMENTS_INTERNAL_ERROR");
    free_token_data(token_data);
    return res;
  }

  // Build response
  char payload[4096] = "LIST_APPOINTMENTS_SUCCESS||";
  int first = 1;

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result))) {
    if (!first)
      strcat(payload, "||");

    char meeting_str[512];
    snprintf(meeting_str, sizeof(meeting_str), "%s&%s&%s&%s&%s", row[0], row[1],
             row[2], row[3], row[4]);
    strcat(payload, meeting_str);

    first = 0;
  }

  if (first) {
    strcpy(payload, "LIST_APPOINTMENTS_SUCCESS||EMPTY");
  }

  res->status_code = STATUS_OK;
  strcpy(res->payload, payload);

  log_message("INFO", "Listed appointments for teacher_id=%d",
              token_data->user_id);

  // Cleanup
  mysql_free_result(result);
  free_token_data(token_data);

  return res;
}

// ============= ADD_MINUTES =============
Response *handle_add_minutes(Request *req, MYSQL *db_conn) {
  Response *res = calloc(1, sizeof(Response));

  // Validate token
  TokenData *token_data = validate_token(req->token);
  if (!token_data) {
    res->status_code = STATUS_TOKEN_INVALID;
    strcpy(res->payload, "ADD_MINUTES_INVALID_TOKEN");
    return res;
  }

  // Check role = teacher
  if (strcmp(token_data->role, "teacher") != 0) {
    res->status_code = STATUS_FORBIDDEN;
    strcpy(res->payload, "ADD_MINUTES_FORBIDDEN");
    free_token_data(token_data);
    return res;
  }

  // Parse data: meeting_id||<base64_content>
  int field_count;
  char **fields = parse_data_fields(req->data, &field_count);

  if (field_count != 2) {
    res->status_code = STATUS_BAD_REQUEST;
    strcpy(res->payload, "ADD_MINUTES_INVALID_FORMAT");
    free_token_data(token_data);
    free_split(fields, field_count);
    return res;
  }

  int meeting_id = atoi(trim(fields[0]));
  char *content = fields[1]; // Plain text content

  // Check meeting exists and belongs to teacher
  char query[512];
  snprintf(query, sizeof(query),
           "SELECT s.teacher_id FROM meetings m "
           "JOIN slots s ON m.slot_id = s.slot_id "
           "WHERE m.meeting_id=%d",
           meeting_id);

  MYSQL_RES *result = db_query(db_conn, query);

  if (!result || mysql_num_rows(result) == 0) {
    res->status_code = STATUS_NOT_FOUND;
    strcpy(res->payload, "ADD_MINUTES_MEETING_NOT_FOUND");
    if (result)
      mysql_free_result(result);
    free_token_data(token_data);
    free_split(fields, field_count);
    return res;
  }

  MYSQL_ROW row = mysql_fetch_row(result);
  int teacher_id = atoi(row[0]);
  mysql_free_result(result);

  if (teacher_id != token_data->user_id) {
    res->status_code = STATUS_FORBIDDEN;
    strcpy(res->payload, "ADD_MINUTES_FORBIDDEN");
    free_token_data(token_data);
    free_split(fields, field_count);
    return res;
  }

  // Save to file: minutes/meeting_<id>.txt
  char filename[256];
  snprintf(filename, sizeof(filename), "minutes/meeting_%d.txt", meeting_id);

  FILE *file = fopen(filename, "w");
  if (!file) {
    res->status_code = STATUS_INTERNAL_ERROR;
    strcpy(res->payload, "ADD_MINUTES_FILE_ERROR");
    free_token_data(token_data);
    free_split(fields, field_count);
    return res;
  }

  fputs(content, file);
  fclose(file);

  // Success
  res->status_code = STATUS_OK;
  strcpy(res->payload, "ADD_MINUTES_SUCCESS");

  log_message("INFO", "Minutes added for meeting_id=%d", meeting_id);

  // Cleanup
  free_token_data(token_data);
  free_split(fields, field_count);

  return res;
}

// ============= GET_MINUTES =============
Response *handle_get_minutes(Request *req, MYSQL *db_conn) {
  (void)db_conn;
  Response *res = calloc(1, sizeof(Response));

  // Validate token
  TokenData *token_data = validate_token(req->token);
  if (!token_data) {
    res->status_code = STATUS_TOKEN_INVALID;
    strcpy(res->payload, "GET_MINUTES_INVALID_TOKEN");
    return res;
  }

  // Parse data: meeting_id
  int meeting_id = atoi(trim(req->data));

  // Read file: minutes/meeting_<id>.txt
  char filename[256];
  snprintf(filename, sizeof(filename), "minutes/meeting_%d.txt", meeting_id);

  FILE *file = fopen(filename, "r");
  if (!file) {
    res->status_code = STATUS_NOT_FOUND;
    strcpy(res->payload, "GET_MINUTES_NOT_FOUND");
    free_token_data(token_data);
    return res;
  }

  // Read content
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  char *content = malloc(file_size + 1);
  fread(content, 1, file_size, file);
  content[file_size] = '\0';
  fclose(file);

  // Build response: GET_MINUTES_SUCCESS||<content>
  res->status_code = STATUS_OK;
  snprintf(res->payload, sizeof(res->payload), "GET_MINUTES_SUCCESS||%s",
           content);

  log_message("INFO", "Minutes retrieved for meeting_id=%d", meeting_id);

  // Cleanup
  free(content);
  free_token_data(token_data);

  return res;
}

// ============= VIEW_HISTORY (Teacher) =============
Response *handle_view_history(Request *req, MYSQL *db_conn) {
  Response *res = calloc(1, sizeof(Response));

  // Validate token
  TokenData *token_data = validate_token(req->token);
  if (!token_data) {
    res->status_code = STATUS_TOKEN_INVALID;
    strcpy(res->payload, "VIEW_HISTORY_INVALID_TOKEN");
    return res;
  }

  // Check role = teacher
  if (strcmp(token_data->role, "teacher") != 0) {
    res->status_code = STATUS_FORBIDDEN;
    strcpy(res->payload, "VIEW_HISTORY_FORBIDDEN");
    free_token_data(token_data);
    return res;
  }

  // Parse data: student_id
  int student_id = atoi(trim(req->data));

  // Query history
  char query[1024];
  snprintf(query, sizeof(query),
           "SELECT m.meeting_id, s.start_time FROM meetings m "
           "JOIN slots s ON m.slot_id = s.slot_id "
           "WHERE m.student_id=%d AND s.teacher_id=%d "
           "ORDER BY s.start_time DESC",
           student_id, token_data->user_id);

  MYSQL_RES *result = db_query(db_conn, query);

  if (!result) {
    res->status_code = STATUS_INTERNAL_ERROR;
    strcpy(res->payload, "VIEW_HISTORY_INTERNAL_ERROR");
    free_token_data(token_data);
    return res;
  }

  // Build response: meeting_id&date&minutes_exist|...
  char payload[4096] = "VIEW_HISTORY_SUCCESS||";
  int first = 1;

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result))) {
    if (!first)
      strcat(payload, "||");

    int meeting_id = atoi(row[0]);

    // Check if minutes exist
    char filename[256];
    snprintf(filename, sizeof(filename), "minutes/meeting_%d.txt", meeting_id);
    FILE *file = fopen(filename, "r");
    int minutes_exist = (file != NULL);
    if (file)
      fclose(file);

    char history_str[256];
    snprintf(history_str, sizeof(history_str), "%s&%s&%d", row[0], row[1],
             minutes_exist);
    strcat(payload, history_str);

    first = 0;
  }

  if (first) {
    strcpy(payload, "VIEW_HISTORY_SUCCESS||EMPTY");
  }

  res->status_code = STATUS_OK;
  strcpy(res->payload, payload);

  log_message("INFO", "Viewed history for student_id=%d", student_id);

  // Cleanup
  mysql_free_result(result);
  free_token_data(token_data);

  return res;
}
