#include "protocol_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Response *parse_response(const char *raw) {
  Response *res = calloc(1, sizeof(Response));
  if (!res)
    return NULL;

  // Find first || delimiter
  const char *delim = strstr(raw, "||");

  if (!delim) {
    free(res);
    return NULL;
  }

  // Parse status code (before ||)
  char status_str[16];
  size_t status_len = delim - raw;
  if (status_len >= sizeof(status_str))
    status_len = sizeof(status_str) - 1;
  strncpy(status_str, raw, status_len);
  status_str[status_len] = '\0';
  res->status_code = atoi(status_str);

  // Parse payload (after ||)
  const char *payload_start = delim + 2;
  strncpy(res->payload, payload_start, sizeof(res->payload) - 1);
  res->payload[sizeof(res->payload) - 1] = '\0';

  // Remove trailing \r\n
  size_t len = strlen(res->payload);
  if (len >= 2 && res->payload[len - 2] == '\r' &&
      res->payload[len - 1] == '\n') {
    res->payload[len - 2] = '\0';
  }

  return res;
}

char *build_request(const char *command, const char *token, const char *data) {
  static char buffer[8192];

  snprintf(buffer, sizeof(buffer), "%s||%s||%s\r\n", command,
           token ? token : "", data ? data : "");

  return buffer;
}

// Parse payload fields - for responses with multiple || delimited fields
char **parse_payload_fields(const char *payload, int *count) {
  if (!payload || !count) {
    if (count)
      *count = 0;
    return NULL;
  }

  *count = 0;

  // Count fields by counting || delimiters
  int field_count = 1;
  const char *p = payload;
  while ((p = strstr(p, "||")) != NULL) {
    field_count++;
    p += 2;
  }

  // Allocate array
  char **fields = malloc(field_count * sizeof(char *));
  if (!fields)
    return NULL;

  // Parse fields
  char *copy = strdup(payload);
  char *start = copy;
  int idx = 0;

  while (idx < field_count) {
    char *delim = strstr(start, "||");
    if (delim) {
      *delim = '\0';
      fields[idx++] = strdup(start);
      start = delim + 2;
    } else {
      // Last field
      fields[idx++] = strdup(start);
      break;
    }
  }

  free(copy);
  *count = idx;
  return fields;
}

void free_response(Response *res) {
  if (res)
    free(res);
}

void free_fields(char **fields, int count) {
  if (!fields)
    return;

  for (int i = 0; i < count; i++) {
    if (fields[i])
      free(fields[i]);
  }
  free(fields);
}
