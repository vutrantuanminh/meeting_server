#include "protocol.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

// ============= PARSE REQUEST =============
Request *parse_request(const char *raw_message) {
  if (!raw_message) {
    log_message("ERROR", "parse_request: raw_message is NULL");
    return NULL;
  }

  Request *req = calloc(1, sizeof(Request));
  if (!req) {
    log_message("ERROR", "parse_request: calloc failed");
    return NULL;
  }

  char *msg_copy = strdup(raw_message);
  char *msg = msg_copy;

  // Remove \r\n
  char *crlf = strstr(msg, "\r\n");
  if (crlf)
    *crlf = '\0';

  // Parse format: COMMAND||TOKEN||DATA
  char *first_delim = strstr(msg, "||");

  if (first_delim) {
    // Extract COMMAND
    *first_delim = '\0';
    strncpy(req->command, msg, sizeof(req->command) - 1);
    msg = first_delim + 2; // Skip ||

    // Extract TOKEN
    char *second_delim = strstr(msg, "||");
    if (second_delim) {
      *second_delim = '\0';
      strncpy(req->token, msg, sizeof(req->token) - 1);
      msg = second_delim + 2; // Skip ||

      // Extract DATA
      strncpy(req->data, msg, sizeof(req->data) - 1);
    } else {
      // No DATA, only TOKEN
      strncpy(req->token, msg, sizeof(req->token) - 1);
    }
  } else {
    // No delimiters, only COMMAND
    strncpy(req->command, msg, sizeof(req->command) - 1);
  }

  free(msg_copy);
  return req;
}

// ============= BUILD RESPONSE =============
char *build_response(int status_code, const char *payload) {
  char *response = malloc(4096);
  if (!response) {
    log_message("ERROR", "build_response: malloc failed!");
    return NULL;
  }

  if (payload && strlen(payload) > 0) {
    snprintf(response, 4096, "%d||%s\r\n", status_code, payload);
  } else {
    snprintf(response, 4096, "%d\r\n", status_code);
  }

  return response;
}

// ============= PARSE SUBFIELDS =============
char **parse_subfields(const char *field, int *subfield_count) {
  *subfield_count = 0;

  if (!field || strlen(field) == 0) {
    return NULL;
  }

  char *field_copy = strdup(field);
  if (!field_copy) {
    log_message("ERROR", "parse_subfields: strdup failed");
    return NULL;
  }

  char **result = split_string(field_copy, "&", subfield_count);
  free(field_copy);

  return result;
}

// ============= FREE FUNCTIONS =============
void free_request(Request *req) {
  if (req)
    free(req);
}

void free_response_string(char *response) {
  if (response)
    free(response);
}

// ============= PARSE DATA FIELDS =============
char **parse_data_fields(const char *data, int *field_count) {
  *field_count = 0;

  if (!data || strlen(data) == 0) {
    return NULL;
  }

  char *data_copy = strdup(data);
  if (!data_copy) {
    log_message("ERROR", "parse_data_fields: strdup failed");
    return NULL;
  }

  char **result = split_string(data_copy, "||", field_count);
  free(data_copy);

  return result;
}