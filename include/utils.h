#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Logging
void log_message(const char* level, const char* format, ...);

// String utilities
char* trim(char* str);
char** split_string(char* str, const char* delimiter, int* count);
void free_split(char** parts, int count);

// Base64 encode/decode
char* base64_encode(const unsigned char* input, int length);
unsigned char* base64_decode(const char* input, int* output_length);

// Generate random string
void generate_random_string(char* dest, int length);

#endif
