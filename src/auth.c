#include "auth.h"
#include "utils.h"
#include <openssl/sha.h>
#include <string.h>
#include <stdlib.h>

// ============= HASH PASSWORD =============
char* hash_user_password(const char* password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)password, strlen(password), hash);
    
    char* hex = malloc(SHA256_DIGEST_LENGTH * 2 + 1);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(hex + (i * 2), "%02x", hash[i]);
    }
    hex[SHA256_DIGEST_LENGTH * 2] = '\0';
    
    return hex;
}

// ============= GENERATE TOKEN =============
char* generate_token(int user_id, const char* username, const char* role) {
    char random_str[17];
    generate_random_string(random_str, 16);
    
    time_t now = time(NULL);
    
    char token_plain[512];
    snprintf(token_plain, sizeof(token_plain), "%d:%s:%s:%ld:%s", 
             user_id, username, role, now, random_str);
    
    char* token_b64 = base64_encode((unsigned char*)token_plain, strlen(token_plain));
    
    return token_b64;
}

// ============= VALIDATE TOKEN =============
TokenData* validate_token(const char* token) {
    if (!token || strlen(token) == 0) {
        return NULL;
    }
    
    int decoded_len = 0;
    unsigned char* decoded = base64_decode(token, &decoded_len);
    
    if (!decoded || decoded_len <= 0) {
        if (decoded) free(decoded);
        return NULL;
    }
    
    char* decoded_str = malloc(decoded_len + 1);
    if (!decoded_str) {
        free(decoded);
        return NULL;
    }
    
    memcpy(decoded_str, decoded, decoded_len);
    decoded_str[decoded_len] = '\0';
    free(decoded);
    
    TokenData* data = malloc(sizeof(TokenData));
    if (!data) {
        free(decoded_str);
        return NULL;
    }
    
    char username[100], role[20];
    long timestamp;
    
    int parsed = sscanf(decoded_str, "%d:%99[^:]:%19[^:]:%ld:", 
                       &data->user_id, username, role, &timestamp);
    
    free(decoded_str);
    
    if (parsed < 4) {
        free(data);
        return NULL;
    }
    
    time_t now = time(NULL);
    if (now - timestamp > 86400) {
        free(data);
        return NULL;
    }
    
    strcpy(data->username, username);
    strcpy(data->role, role);
    data->created_at = timestamp;
    
    return data;
}

// ============= FREE =============
void free_token_data(TokenData* data) {
    if (data) {
        free(data);
    }
}