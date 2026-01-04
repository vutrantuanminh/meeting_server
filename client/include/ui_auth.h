#ifndef UI_AUTH_H
#define UI_AUTH_H

#include <stdbool.h>

// Auth result structure
typedef struct {
    char token[512];
    char username[64];
    char role[16];
} AuthResult;

// Auth screens
AuthResult* show_login_screen(int sockfd);
bool show_register_screen(int sockfd);
int show_welcome_screen();

// Free auth result
void free_auth_result(AuthResult* auth);

#endif
