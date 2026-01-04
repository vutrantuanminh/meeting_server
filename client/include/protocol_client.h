#ifndef PROTOCOL_CLIENT_H
#define PROTOCOL_CLIENT_H

#include <stdbool.h>

// Status codes (same as server)
#define STATUS_OK                    2000
#define STATUS_CHUNK_OK              2001
#define STATUS_BAD_REQUEST           4000
#define STATUS_CONFLICT              4001
#define STATUS_TOKEN_MISSING         4010
#define STATUS_TOKEN_INVALID         4011
#define STATUS_FORBIDDEN             4030
#define STATUS_NOT_FOUND             4040
#define STATUS_WRONG_PASSWORD        4041
#define STATUS_USERNAME_EXISTS       4090
#define STATUS_INTERNAL_ERROR        5000

// Response structure
typedef struct {
    int status_code;
    char payload[4096];
} Response;

// Parse response from server
Response* parse_response(const char* raw_message);

// Build request string
char* build_request(const char* command, const char* token, const char* data);

// Parse payload fields (split by ||)
char** parse_payload_fields(const char* payload, int* field_count);

// Free functions
void free_response(Response* res);
void free_fields(char** fields, int count);

#endif
