#include "network.h"
#include "ui_auth.h"
#include "ui_core.h"
#include "ui_student.h"
#include "ui_teacher.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>

static int sockfd = -1;

void cleanup_and_exit(int sig) {
  (void)sig;
  cleanup_ui();
  if (sockfd >= 0) {
    close_connection(sockfd);
  }
  exit(0);
}

int main(void) {
  // Setup signal handler
  signal(SIGINT, cleanup_and_exit);
  signal(SIGTERM, cleanup_and_exit);

  // Initialize UI
  init_ui();

  // Connect to server
  sockfd = connect_to_server(SERVER_HOST, SERVER_PORT);
  if (sockfd < 0) {
    cleanup_ui();
    fprintf(stderr, "Failed to connect to server at %s:%d\n", SERVER_HOST,
            SERVER_PORT);
    fprintf(stderr, "Make sure the server is running.\n");
    return 1;
  }

  // Main application loop
  while (1) {
    clear_screen();

    // Show welcome screen
    int choice = show_welcome_screen();

    if (choice == 2 || choice == -1) { // Exit
      break;
    }

    if (choice == 0) { // Login
      AuthResult *auth = show_login_screen(sockfd);

      if (auth) {
        // Show appropriate menu based on role
        if (strcmp(auth->role, "student") == 0) {
          show_student_menu(sockfd, auth->token, auth->username);
        } else if (strcmp(auth->role, "teacher") == 0) {
          show_teacher_menu(sockfd, auth->token, auth->username);
        } else {
          show_error("Unknown role");
          napms(2000);
        }

        free_auth_result(auth);
      }

    } else if (choice == 1) { // Register
      show_register_screen(sockfd);
    }
  }

  // Cleanup
  cleanup_ui();
  close_connection(sockfd);

  return 0;
}
