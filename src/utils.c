#include "utils.h"
#include <stdarg.h>
#include <ctype.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

// ============= LOGGING =============
void log_message(const char* level, const char* format, ...) {
    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    fprintf(stdout, "[%s] [%s] ", timestamp, level);
    
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
    
    fprintf(stdout, "\n");
    fflush(stdout);
}

// ============= STRING UTILITIES =============
char* trim(char* str) {
    if (!str) return NULL;
    
    while(isspace((unsigned char)*str)) str++;
    
    if(*str == 0) return str;
    
    char* end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    
    *(end + 1) = '\0';
    
    return str;
}

char** split_string(char* str, const char* delimiter, int* count) {
    if (!str || !delimiter) {
        *count = 0;
        return NULL;
    }
    
    int max_parts = 50;
    char** parts = malloc(sizeof(char*) * max_parts);
    if (!parts) {
        *count = 0;
        return NULL;
    }
    
    *count = 0;
    
    char* str_copy = strdup(str);
    if (!str_copy) {
        free(parts);
        *count = 0;
        return NULL;
    }
    
    char* token = strtok(str_copy, delimiter);
    
    while (token != NULL && *count < max_parts) {
        parts[*count] = strdup(token);
        if (!parts[*count]) {
            for (int i = 0; i < *count; i++) {
                free(parts[i]);
            }
            free(parts);
            free(str_copy);
            *count = 0;
            return NULL;
        }
        (*count)++;
        token = strtok(NULL, delimiter);
    }
    
    free(str_copy);
    return parts;
}

void free_split(char** parts, int count) {
    if (!parts) return;
    
    for (int i = 0; i < count; i++) {
        if (parts[i]) {
            free(parts[i]);
        }
    }
    free(parts);
}

// ============= BASE64 =============
char* base64_encode(const unsigned char* input, int length) {
    BIO *bio, *b64;
    BUF_MEM *buffer_ptr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, input, length);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &buffer_ptr);

    char* result = malloc(buffer_ptr->length + 1);
    memcpy(result, buffer_ptr->data, buffer_ptr->length);
    result[buffer_ptr->length] = '\0';

    BIO_free_all(bio);
    return result;
}

unsigned char* base64_decode(const char* input, int* output_length) {
    BIO *bio, *b64;
    int decode_len = strlen(input);
    unsigned char* buffer = malloc(decode_len);

    bio = BIO_new_mem_buf(input, -1);
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    *output_length = BIO_read(bio, buffer, decode_len);

    BIO_free_all(bio);
    return buffer;
}

// ============= RANDOM STRING =============
void generate_random_string(char* dest, int length) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    srand(time(NULL));
    
    for (int i = 0; i < length; i++) {
        int key = rand() % (sizeof(charset) - 1);
        dest[i] = charset[key];
    }
    dest[length] = '\0';
}