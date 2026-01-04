#ifndef AUTH_H
#define AUTH_H

#include <mysql/mysql.h>
#include <time.h>

// Token structure
typedef struct {
    int user_id;
    char username[100];
    char role[20];  // "teacher" hoặc "student"
    time_t created_at;
} TokenData;

// Hash password với SHA256 - ĐỔI TÊN ĐỂ TRÁNH XUNG ĐỘT
char* hash_user_password(const char* password);

// Generate token: base64(user_id:timestamp:random)
char* generate_token(int user_id, const char* username, const char* role);

// Validate token và trả về user info
TokenData* validate_token(const char* token);

// Free token data
void free_token_data(TokenData* data);

#endif
