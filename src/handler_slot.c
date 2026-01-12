#include "handler_slot.h"
#include "auth.h"
#include "database.h"
#include "protocol.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

// ============= ADD_SLOT =============
Response *handle_add_slot(Request *req, MYSQL *db_conn) {
  Response *res = malloc(sizeof(Response));
  if (!res) {
    log_message("ERROR", "ADD_SLOT: malloc failed");
    return NULL;
  }
  memset(res, 0, sizeof(Response));

  TokenData *token_data = validate_token(req->token);
  if (!token_data) {
    res->status_code = STATUS_TOKEN_INVALID;
    strcpy(res->payload, "ADD_SLOT_INVALID_TOKEN");
    return res;
  }

  if (strcmp(token_data->role, "teacher") != 0) {
    res->status_code = STATUS_FORBIDDEN;
    strcpy(res->payload, "ADD_SLOT_FORBIDDEN");
    free_token_data(token_data);
    return res;
  }

  // Parse data: date||start_time||end_time||slot_type
  int field_count;
  char **fields = parse_data_fields(req->data, &field_count);

  if (field_count != 4) {
    res->status_code = STATUS_BAD_REQUEST;
    strcpy(res->payload, "ADD_SLOT_INVALID_FORMAT");
    if (fields)
      free_split(fields, field_count);
    free_token_data(token_data);
    return res;
  }

  char *date = trim(fields[0]);
  char *start_time_only = trim(fields[1]);
  char *end_time_only = trim(fields[2]);
  int slot_type = atoi(trim(fields[3]));

  if (slot_type < 0 || slot_type > 2) {
    res->status_code = STATUS_BAD_REQUEST;
    strcpy(res->payload, "ADD_SLOT_INVALID_TYPE");
    free_split(fields, field_count);
    free_token_data(token_data);
    return res;
  }

  // Combine date + time: "YYYY-MM-DD HH:MM:SS"
  char start_time[64], end_time[64];
  snprintf(start_time, sizeof(start_time), "%s %s:00", date, start_time_only);
  snprintf(end_time, sizeof(end_time), "%s %s:00", date, end_time_only);

  char query[1024];
  snprintf(query, sizeof(query),
           "SELECT slot_id FROM slots WHERE teacher_id=%d AND ("
           "(start_time <= '%s' AND end_time > '%s') OR "
           "(start_time < '%s' AND end_time >= '%s'))",
           token_data->user_id, start_time, start_time, end_time, end_time);

  MYSQL_RES *result = db_query(db_conn, query);

  if (result == NULL && mysql_errno(db_conn) != 0) {
    res->status_code = STATUS_INTERNAL_ERROR;
    strcpy(res->payload, "ADD_SLOT_INTERNAL_ERROR");
    free_split(fields, field_count);
    free_token_data(token_data);
    return res;
  }

  int has_overlap = 0;
  if (result) {
    has_overlap = (mysql_num_rows(result) > 0);
    mysql_free_result(result);
  }

  if (has_overlap) {
    res->status_code = STATUS_USERNAME_EXISTS;
    strcpy(res->payload, "ADD_SLOT_TIME_OVERLAP");
    free_split(fields, field_count);
    free_token_data(token_data);
    return res;
  }

  snprintf(query, sizeof(query),
           "INSERT INTO slots (teacher_id, start_time, end_time, slot_type) "
           "VALUES (%d, '%s', '%s', %d)",
           token_data->user_id, start_time, end_time, slot_type);

  int affected = db_execute(db_conn, query);

  if (affected <= 0) {
    res->status_code = STATUS_INTERNAL_ERROR;
    strcpy(res->payload, "ADD_SLOT_INTERNAL_ERROR");
    free_split(fields, field_count);
    free_token_data(token_data);
    return res;
  }

  int slot_id = mysql_insert_id(db_conn);

  res->status_code = STATUS_OK;
  snprintf(res->payload, sizeof(res->payload), "ADD_SLOT_SUCCESS||%d", slot_id);

  log_message("INFO", "Slot added: id=%d by teacher=%d", slot_id,
              token_data->user_id);

  free_split(fields, field_count);
  free_token_data(token_data);

  return res;
}

// ============= UPDATE_SLOT =============
Response *handle_update_slot(Request *req, MYSQL *db_conn) {
  Response *res = calloc(1, sizeof(Response));

  TokenData *token_data = validate_token(req->token);
  if (!token_data) {
    res->status_code = STATUS_TOKEN_INVALID;
    strcpy(res->payload, "UPDATE_SLOT_INVALID_TOKEN");
    return res;
  }

  if (strcmp(token_data->role, "teacher") != 0) {
    res->status_code = STATUS_FORBIDDEN;
    strcpy(res->payload, "UPDATE_SLOT_FORBIDDEN");
    free_token_data(token_data);
    return res;
  }

  int field_count;
  char **fields = parse_subfields(req->data, &field_count);

  if (field_count != 4) {
    res->status_code = STATUS_BAD_REQUEST;
    strcpy(res->payload, "UPDATE_SLOT_INVALID_FORMAT");
    free_token_data(token_data);
    free_split(fields, field_count);
    return res;
  }

  int slot_id = atoi(trim(fields[0]));
  char *start_time = trim(fields[1]);
  char *end_time = trim(fields[2]);
  int slot_type = atoi(trim(fields[3]));

  char query[1024];
  snprintf(query, sizeof(query),
           "SELECT slot_id FROM slots WHERE slot_id=%d AND teacher_id=%d",
           slot_id, token_data->user_id);

  MYSQL_RES *result = db_query(db_conn, query);
  if (!result || mysql_num_rows(result) == 0) {
    res->status_code = STATUS_NOT_FOUND;
    strcpy(res->payload, "UPDATE_SLOT_NOT_FOUND");
    if (result)
      mysql_free_result(result);
    free_token_data(token_data);
    free_split(fields, field_count);
    return res;
  }
  mysql_free_result(result);

  snprintf(query, sizeof(query),
           "UPDATE slots SET start_time='%s', end_time='%s', slot_type=%d "
           "WHERE slot_id=%d",
           start_time, end_time, slot_type, slot_id);

  int affected = db_execute(db_conn, query);

  if (affected < 0) {
    res->status_code = STATUS_INTERNAL_ERROR;
    strcpy(res->payload, "UPDATE_SLOT_INTERNAL_ERROR");
    free_token_data(token_data);
    free_split(fields, field_count);
    return res;
  }

  res->status_code = STATUS_OK;
  strcpy(res->payload, "UPDATE_SLOT_SUCCESS");

  log_message("INFO", "Slot updated: id=%d", slot_id);

  free_token_data(token_data);
  free_split(fields, field_count);

  return res;
}

// ============= DELETE_SLOT =============
Response *handle_delete_slot(Request *req, MYSQL *db_conn) {
  Response *res = calloc(1, sizeof(Response));

  TokenData *token_data = validate_token(req->token);
  if (!token_data) {
    res->status_code = STATUS_TOKEN_INVALID;
    strcpy(res->payload, "DELETE_SLOT_INVALID_TOKEN");
    return res;
  }

  if (strcmp(token_data->role, "teacher") != 0) {
    res->status_code = STATUS_FORBIDDEN;
    strcpy(res->payload, "DELETE_SLOT_FORBIDDEN");
    free_token_data(token_data);
    return res;
  }

  int slot_id = atoi(trim(req->data));

  char query[512];
  snprintf(query, sizeof(query),
           "SELECT is_booked FROM slots WHERE slot_id=%d AND teacher_id=%d",
           slot_id, token_data->user_id);

  MYSQL_RES *result = db_query(db_conn, query);
  if (!result || mysql_num_rows(result) == 0) {
    res->status_code = STATUS_NOT_FOUND;
    strcpy(res->payload, "DELETE_SLOT_NOT_FOUND");
    if (result)
      mysql_free_result(result);
    free_token_data(token_data);
    return res;
  }

  MYSQL_ROW row = mysql_fetch_row(result);
  int is_booked = atoi(row[0]);
  mysql_free_result(result);

  if (is_booked) {
    res->status_code = STATUS_USERNAME_EXISTS;
    strcpy(res->payload, "DELETE_SLOT_IN_USE");
    free_token_data(token_data);
    return res;
  }

  snprintf(query, sizeof(query), "DELETE FROM slots WHERE slot_id=%d", slot_id);

  int affected = db_execute(db_conn, query);

  if (affected <= 0) {
    res->status_code = STATUS_INTERNAL_ERROR;
    strcpy(res->payload, "DELETE_SLOT_INTERNAL_ERROR");
    free_token_data(token_data);
    return res;
  }

  res->status_code = STATUS_OK;
  strcpy(res->payload, "DELETE_SLOT_SUCCESS");

  log_message("INFO", "Slot deleted: id=%d", slot_id);

  free_token_data(token_data);

  return res;
}

// ============= LIST_FREE_SLOTS =============
Response *handle_list_free_slots(Request *req, MYSQL *db_conn) {
  Response *res = calloc(1, sizeof(Response));

  TokenData *token_data = validate_token(req->token);
  if (!token_data) {
    res->status_code = STATUS_TOKEN_INVALID;
    strcpy(res->payload, "LIST_FREE_SLOTS_INVALID_TOKEN");
    return res;
  }

  int teacher_id = atoi(trim(req->data));

  char query[512];
  if (teacher_id == 0) {
    snprintf(
        query, sizeof(query),
        "SELECT s.slot_id, s.teacher_id, u.username, s.start_time, s.end_time, "
        "CASE s.slot_type WHEN 0 THEN 'Individual' WHEN 1 THEN 'Group' ELSE "
        "'Both' END "
        "FROM slots s JOIN users u ON s.teacher_id = u.user_id "
        "WHERE s.is_booked=0 ORDER BY s.start_time");
  } else {
    snprintf(
        query, sizeof(query),
        "SELECT s.slot_id, s.teacher_id, u.username, s.start_time, s.end_time, "
        "CASE s.slot_type WHEN 0 THEN 'Individual' WHEN 1 THEN 'Group' ELSE "
        "'Both' END "
        "FROM slots s JOIN users u ON s.teacher_id = u.user_id "
        "WHERE s.teacher_id=%d AND s.is_booked=0 ORDER BY s.start_time",
        teacher_id);
  }

  MYSQL_RES *result = db_query(db_conn, query);

  if (!result) {
    res->status_code = STATUS_INTERNAL_ERROR;
    strcpy(res->payload, "LIST_FREE_SLOTS_INTERNAL_ERROR");
    free_token_data(token_data);
    return res;
  }

  char payload[4096] = "LIST_FREE_SLOTS_SUCCESS||";
  int first = 1;

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result))) {
    if (!first)
      strcat(payload, "||");

    char slot_str[256];
    snprintf(slot_str, sizeof(slot_str), "%s&%s&%s&%s&%s&%s", row[0], row[1],
             row[2], row[3], row[4], row[5]);
    strcat(payload, slot_str);

    first = 0;
  }

  if (first) {
    strcpy(payload, "LIST_FREE_SLOTS_SUCCESS||EMPTY");
  }

  res->status_code = STATUS_OK;
  strcpy(res->payload, payload);

  log_message("INFO", "Listed free slots for teacher_id=%d", teacher_id);

  mysql_free_result(result);
  free_token_data(token_data);

  return res;
}

// ============= LIST_MY_SLOTS (Teacher's own slots) =============
Response *handle_list_my_slots(Request *req, MYSQL *db_conn) {
  Response *res = calloc(1, sizeof(Response));

  TokenData *token_data = validate_token(req->token);
  if (!token_data) {
    res->status_code = STATUS_TOKEN_INVALID;
    strcpy(res->payload, "LIST_MY_SLOTS_INVALID_TOKEN");
    return res;
  }

  // Must be a teacher
  if (strcmp(token_data->role, "teacher") != 0) {
    res->status_code = STATUS_FORBIDDEN;
    strcpy(res->payload, "LIST_MY_SLOTS_FORBIDDEN");
    free_token_data(token_data);
    return res;
  }

  char query[512];
  snprintf(
      query, sizeof(query),
      "SELECT slot_id, DATE(start_time), TIME(start_time), TIME(end_time), "
      "CASE slot_type WHEN 0 THEN 'Individual' WHEN 1 THEN 'Group' ELSE 'Both' "
      "END, "
      "is_booked FROM slots WHERE teacher_id=%d ORDER BY start_time",
      token_data->user_id);

  MYSQL_RES *result = db_query(db_conn, query);

  if (!result) {
    res->status_code = STATUS_INTERNAL_ERROR;
    strcpy(res->payload, "LIST_MY_SLOTS_INTERNAL_ERROR");
    free_token_data(token_data);
    return res;
  }

  char payload[4096] = "LIST_MY_SLOTS_SUCCESS||";
  int first = 1;

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result))) {
    if (!first)
      strcat(payload, "||");

    // slot_id&date&start_time&end_time&type&is_booked
    char slot_str[256];
    snprintf(slot_str, sizeof(slot_str), "%s&%s&%s&%s&%s&%s", row[0], row[1],
             row[2], row[3], row[4], row[5]);
    strcat(payload, slot_str);

    first = 0;
  }

  if (first) {
    strcpy(payload, "LIST_MY_SLOTS_SUCCESS||EMPTY");
  }

  res->status_code = STATUS_OK;
  strcpy(res->payload, payload);

  log_message("INFO", "Listed slots for teacher_id=%d", token_data->user_id);

  mysql_free_result(result);
  free_token_data(token_data);

  return res;
}

// ============= LIST_STUDENTS =============
Response *handle_list_students(Request *req, MYSQL *db_conn) {
  Response *res = calloc(1, sizeof(Response));

  TokenData *token_data = validate_token(req->token);
  if (!token_data) {
    res->status_code = STATUS_TOKEN_INVALID;
    strcpy(res->payload, "LIST_STUDENTS_INVALID_TOKEN");
    return res;
  }

  // Must be a teacher
  if (strcmp(token_data->role, "teacher") != 0) {
    res->status_code = STATUS_FORBIDDEN;
    strcpy(res->payload, "LIST_STUDENTS_FORBIDDEN");
    free_token_data(token_data);
    return res;
  }

  // Only get students who have meetings with this teacher
  char query[512];
  snprintf(query, sizeof(query),
           "SELECT DISTINCT u.user_id, u.username "
           "FROM users u "
           "JOIN meetings m ON u.user_id = m.student_id "
           "JOIN slots s ON m.slot_id = s.slot_id "
           "WHERE s.teacher_id = %d "
           "UNION "
           "SELECT DISTINCT u.user_id, u.username "
           "FROM users u "
           "JOIN group_members gm ON u.user_id = gm.student_id "
           "JOIN meetings m ON gm.meeting_id = m.meeting_id "
           "JOIN slots s ON m.slot_id = s.slot_id "
           "WHERE s.teacher_id = %d "
           "ORDER BY username",
           token_data->user_id, token_data->user_id);

  MYSQL_RES *result = db_query(db_conn, query);

  if (!result) {
    res->status_code = STATUS_INTERNAL_ERROR;
    strcpy(res->payload, "LIST_STUDENTS_INTERNAL_ERROR");
    free_token_data(token_data);
    return res;
  }

  char payload[4096] = "LIST_STUDENTS_SUCCESS||";
  int first = 1;

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result))) {
    if (!first)
      strcat(payload, "||");

    // user_id&username
    char student_str[128];
    snprintf(student_str, sizeof(student_str), "%s&%s", row[0], row[1]);
    strcat(payload, student_str);

    first = 0;
  }

  if (first) {
    strcpy(payload, "LIST_STUDENTS_SUCCESS||EMPTY");
  }

  res->status_code = STATUS_OK;
  strcpy(res->payload, payload);

  log_message("INFO", "Listed students with meetings for teacher_id=%d",
              token_data->user_id);

  mysql_free_result(result);
  free_token_data(token_data);

  return res;
}

// ============= LIST_ALL_STUDENTS (for group booking) =============
Response *handle_list_all_students(Request *req, MYSQL *db_conn) {
  Response *res = calloc(1, sizeof(Response));

  TokenData *token_data = validate_token(req->token);
  if (!token_data) {
    res->status_code = STATUS_TOKEN_INVALID;
    strcpy(res->payload, "LIST_ALL_STUDENTS_INVALID_TOKEN");
    return res;
  }

  // Get all students except the current user
  char query[256];
  snprintf(query, sizeof(query),
           "SELECT user_id, username FROM users "
           "WHERE role='student' AND user_id != %d "
           "ORDER BY username",
           token_data->user_id);

  MYSQL_RES *result = db_query(db_conn, query);

  if (!result) {
    res->status_code = STATUS_INTERNAL_ERROR;
    strcpy(res->payload, "LIST_ALL_STUDENTS_INTERNAL_ERROR");
    free_token_data(token_data);
    return res;
  }

  char payload[4096] = "LIST_ALL_STUDENTS_SUCCESS||";
  int first = 1;

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result))) {
    if (!first)
      strcat(payload, "||");

    char student_str[128];
    snprintf(student_str, sizeof(student_str), "%s&%s", row[0], row[1]);
    strcat(payload, student_str);

    first = 0;
  }

  if (first) {
    strcpy(payload, "LIST_ALL_STUDENTS_SUCCESS||EMPTY");
  }

  res->status_code = STATUS_OK;
  strcpy(res->payload, payload);

  log_message("INFO", "Listed all students for user_id=%d",
              token_data->user_id);

  mysql_free_result(result);
  free_token_data(token_data);

  return res;
}