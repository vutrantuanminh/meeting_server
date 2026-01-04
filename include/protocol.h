#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>

// Status codes
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

// Request structure
typedef struct {
    char command[32];
    char token[512];
    char data[4096];
} Request;

// Response structure
typedef struct {
    int status_code;
    char payload[4096];
} Response;

// Main functions
Request* parse_request(const char* raw_message);
char* build_response(int status_code, const char* payload);

// Helper functions - ⚠️ ĐẢM BẢO CÓ 2 DÒNG NÀY
char** parse_data_fields(const char* data, int* field_count);
char** parse_subfields(const char* field, int* subfield_count);

// Free functions
void free_request(Request* req);
void free_response_string(char* response);

#endif