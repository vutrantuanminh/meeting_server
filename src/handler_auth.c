#include "handler_auth.h"
#include "auth.h"
#include "database.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

// ============= REGISTER =============
Response *handle_register(Request *req, MYSQL *db_conn) {
  Response *res = calloc(1, sizeof(Response));

  // Parse data: username||password||role
  int field_count;
  char **fields = parse_data_fields(req->data, &field_count);

  if (field_count != 3) {
    res->status_code = STATUS_BAD_REQUEST;
    strcpy(res->payload, "REGISTER_INVALID_FORMAT");
    free_split(fields, field_count);
    return res;
  }

  char *username = trim(fields[0]);
  char *password = trim(fields[1]);
  char *role = trim(fields[2]);

  // Validate
  if (strlen(username) == 0 || strlen(password) == 0 || strlen(role) == 0) {
    res->status_code = STATUS_BAD_REQUEST;
    strcpy(res->payload, "REGISTER_INVALID_FORMAT");
    free_split(fields, field_count);
    return res;
  }

  // Validate role
  if (strcmp(role, "student") != 0 && strcmp(role, "teacher") != 0) {
    res->status_code = STATUS_BAD_REQUEST;
    strcpy(res->payload, "REGISTER_INVALID_ROLE");
    free_split(fields, field_count);
    return res;
  }

  // Check username exists
  char query[512];
  snprintf(query, sizeof(query),
           "SELECT user_id FROM users WHERE username='%s'", username);

  MYSQL_RES *result = db_query(db_conn, query);
  if (result && mysql_num_rows(result) > 0) {
    res->status_code = STATUS_USERNAME_EXISTS;
    strcpy(res->payload, "REGISTER_USERNAME_EXISTS");
    mysql_free_result(result);
    free_split(fields, field_count);
    return res;
  }
  if (result)
    mysql_free_result(result);

  // Hash password
  char *password_hash = hash_user_password(password);

  // Insert new user with selected role
  snprintf(query, sizeof(query),
           "INSERT INTO users (username, password_hash, role) VALUES ('%s', "
           "'%s', '%s')",
           username, password_hash, role);

  int affected = db_execute(db_conn, query);

  if (affected <= 0) {
    res->status_code = STATUS_INTERNAL_ERROR;
    strcpy(res->payload, "REGISTER_INTERNAL_ERROR");
    free(password_hash);
    free_split(fields, field_count);
    return res;
  }

  // Get user_id
  int user_id = mysql_insert_id(db_conn);

  // Generate token with selected role
  char *token = generate_token(user_id, username, role);

  // Build response: REGISTER_SUCCESS||<token>||<role>
  res->status_code = STATUS_OK;
  snprintf(res->payload, sizeof(res->payload), "REGISTER_SUCCESS||%s||%s",
           token, role);

  log_message("INFO", "User registered: %s (id=%d, role=%s)", username, user_id,
              role);

  // Cleanup
  free(password_hash);
  free(token);
  free_split(fields, field_count);

  return res;
}

// ============= LOGIN =============
Response *handle_login(Request *req, MYSQL *db_conn) {
  Response *res = calloc(1, sizeof(Response));

  // Parse data: username&password
  int field_count;
  char **fields = parse_subfields(req->data, &field_count);

  if (field_count != 2) {
    res->status_code = STATUS_BAD_REQUEST;
    strcpy(res->payload, "LOGIN_INVALID_FORMAT");
    free_split(fields, field_count);
    return res;
  }

  char *username = trim(fields[0]);
  char *password = trim(fields[1]);

  // Hash password
  char *password_hash = hash_user_password(password);

  // Query user
  char query[512];
  snprintf(query, sizeof(query),
           "SELECT user_id, role FROM users WHERE username='%s' AND "
           "password_hash='%s'",
           username, password_hash);

  MYSQL_RES *result = db_query(db_conn, query);

  if (!result || mysql_num_rows(result) == 0) {
    // Check if user exists
    snprintf(query, sizeof(query),
             "SELECT user_id FROM users WHERE username='%s'", username);
    MYSQL_RES *check = db_query(db_conn, query);

    if (!check || mysql_num_rows(check) == 0) {
      res->status_code = STATUS_NOT_FOUND;
      strcpy(res->payload, "LOGIN_USER_NOT_FOUND");
    } else {
      res->status_code = STATUS_WRONG_PASSWORD;
      strcpy(res->payload, "LOGIN_WRONG_PASSWORD");
    }

    if (check)
      mysql_free_result(check);
    if (result)
      mysql_free_result(result);
    free(password_hash);
    free_split(fields, field_count);
    return res;
  }

  // Get user info
  MYSQL_ROW row = mysql_fetch_row(result);
  int user_id = atoi(row[0]);
  char *role = row[1];

  // Generate token
  char *token = generate_token(user_id, username, role);

  // Build response: LOGIN_SUCCESS||<token>||<role>
  res->status_code = STATUS_OK;
  snprintf(res->payload, sizeof(res->payload), "LOGIN_SUCCESS||%s||%s", token,
           role);

  log_message("INFO", "User logged in: %s (id=%d, role=%s)", username, user_id,
              role);

  // Cleanup
  mysql_free_result(result);
  free(password_hash);
  free(token);
  free_split(fields, field_count);

  return res;
}

// ============= LOGOUT =============
Response *handle_logout(Request *req, MYSQL *db_conn) {
  (void)db_conn; // Unused

  Response *res = calloc(1, sizeof(Response));

  // Validate token
  TokenData *token_data = validate_token(req->token);

  if (!token_data) {
    res->status_code = STATUS_FORBIDDEN;
    strcpy(res->payload, "LOGOUT_NOT_LOGGED_IN");
    return res;
  }

  // Logout success (chỉ cần client xóa token)
  res->status_code = STATUS_OK;
  strcpy(res->payload, "LOGOUT_SUCCESS");

  log_message("INFO", "User logged out: %s", token_data->username);

  free_token_data(token_data);
  return res;
}
