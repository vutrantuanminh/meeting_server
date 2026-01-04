#include "ui_auth.h"
#include "network.h"
#include "protocol_client.h"
#include "ui_components.h"
#include "ui_core.h"
#include <stdlib.h>
#include <string.h>

int show_welcome_screen() {
  const char *items[] = {"Login", "Register", "Exit"};

  return show_menu("MEETING MANAGEMENT SYSTEM", items, 3);
}

AuthResult *show_login_screen(int sockfd) {
  clear_screen();
  draw_header("LOGIN");

  // Get username
  char *username = show_input_form("Username:", false);
  if (!username)
    return NULL;

  // Get password
  char *password = show_input_form("Password:", true);
  if (!password) {
    free(username);
    return NULL;
  }

  // Build data: username&password
  char data[512];
  snprintf(data, sizeof(data), "%s&%s", username, password);

  // Send LOGIN request
  show_info("Logging in...");

  if (send_request(sockfd, "LOGIN", "", data) < 0) {
    show_error("Failed to send request");
    free(username);
    free(password);
    return NULL;
  }

  // Receive response
  char *raw_response = receive_response(sockfd);
  if (!raw_response) {
    show_error("No response from server");
    free(username);
    free(password);
    return NULL;
  }

  Response *res = parse_response(raw_response);
  if (!res) {
    show_error("Invalid response format");
    free(username);
    free(password);
    return NULL;
  }

  // Check status
  if (res->status_code != STATUS_OK) {
    if (res->status_code == STATUS_NOT_FOUND) {
      show_error("User not found");
    } else if (res->status_code == STATUS_WRONG_PASSWORD) {
      show_error("Wrong password");
    } else {
      show_error(res->payload);
    }
    free_response(res);
    free(username);
    free(password);
    napms(2000);
    return NULL;
  }

  // Parse payload: LOGIN_SUCCESS||<token>||<role>
  int field_count;
  char **fields = parse_payload_fields(res->payload, &field_count);

  if (field_count < 3) {
    show_error("Invalid login response");
    free_fields(fields, field_count);
    free_response(res);
    free(username);
    free(password);
    napms(2000);
    return NULL;
  }

  // Create auth result
  AuthResult *auth = malloc(sizeof(AuthResult));
  strncpy(auth->token, fields[1], sizeof(auth->token) - 1);
  strncpy(auth->username, username, sizeof(auth->username) - 1);
  strncpy(auth->role, fields[2], sizeof(auth->role) - 1);

  show_success("Login successful!");
  napms(1000);

  // Cleanup
  free_fields(fields, field_count);
  free_response(res);
  free(username);
  free(password);

  return auth;
}

bool show_register_screen(int sockfd) {
  clear_screen();
  draw_header("REGISTER");

  // Get username
  char *username = show_input_form("Username:", false);
  if (!username)
    return false;

  // Get password
  char *password = show_input_form("Password:", true);
  if (!password) {
    free(username);
    return false;
  }

  // Get password confirmation
  char *confirm = show_input_form("Confirm Password:", true);
  if (!confirm) {
    free(username);
    free(password);
    return false;
  }

  // Check password match
  if (strcmp(password, confirm) != 0) {
    show_error("Passwords do not match");
    free(username);
    free(password);
    free(confirm);
    napms(2000);
    return false;
  }

  // Select role
  const char *role_items[] = {"Student", "Teacher"};
  int role_choice = show_menu("Select Role", role_items, 2);

  if (role_choice == -1) {
    // User cancelled
    free(username);
    free(password);
    free(confirm);
    return false;
  }

  const char *role = (role_choice == 0) ? "student" : "teacher";

  // Build data: username||password||role
  char data[512];
  snprintf(data, sizeof(data), "%s||%s||%s", username, password, role);

  // Send REGISTER request
  show_info("Registering...");

  if (send_request(sockfd, "REGISTER", "", data) < 0) {
    show_error("Failed to send request");
    free(username);
    free(password);
    free(confirm);
    return false;
  }

  // Receive response
  char *raw_response = receive_response(sockfd);
  if (!raw_response) {
    show_error("No response from server");
    free(username);
    free(password);
    free(confirm);
    return false;
  }

  Response *res = parse_response(raw_response);
  if (!res) {
    show_error("Invalid response format");
    free(username);
    free(password);
    free(confirm);
    return false;
  }

  // Check status
  if (res->status_code != STATUS_OK) {
    if (res->status_code == STATUS_USERNAME_EXISTS) {
      show_error("Username already exists");
    } else {
      show_error(res->payload);
    }
    free_response(res);
    free(username);
    free(password);
    free(confirm);
    napms(2000);
    return false;
  }

  show_success("Registration successful! You can now login.");
  napms(2000);

  // Cleanup
  free_response(res);
  free(username);
  free(password);
  free(confirm);

  return true;
}

void free_auth_result(AuthResult *auth) {
  if (auth)
    free(auth);
}
