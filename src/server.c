#include "server.h"
#include "database.h"
#include "handler_auth.h"
#include "handler_meeting.h"
#include "handler_slot.h"
#include "protocol.h"
#include "utils.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// Global client counter
static int client_counter = 0;

// ============= READ LINE =============
int read_line(int fd, char *buffer, int max_len) {
  int total = 0;
  char c;

  while (total < max_len - 1) {
    int n = read(fd, &c, 1);

    if (n <= 0) {
      if (total == 0)
        return -1;
      break;
    }

    buffer[total++] = c;

    if (total >= 2 && buffer[total - 2] == '\r' && buffer[total - 1] == '\n') {
      buffer[total] = '\0';
      return total;
    }
  }

  buffer[total] = '\0';
  return total;
}

// ============= PROCESS COMMAND =============
Response *process_command(Request *req, MYSQL *db_conn) {
  Response *res = calloc(1, sizeof(Response));

  // AUTH COMMANDS
  if (strcmp(req->command, "REGISTER") == 0) {
    return handle_register(req, db_conn);
  } else if (strcmp(req->command, "LOGIN") == 0) {
    return handle_login(req, db_conn);
  } else if (strcmp(req->command, "LOGOUT") == 0) {
    return handle_logout(req, db_conn);
  }

  // SLOT COMMANDS
  else if (strcmp(req->command, "ADD_SLOT") == 0) {
    return handle_add_slot(req, db_conn);
  } else if (strcmp(req->command, "UPDATE_SLOT") == 0) {
    return handle_update_slot(req, db_conn);
  } else if (strcmp(req->command, "DELETE_SLOT") == 0) {
    return handle_delete_slot(req, db_conn);
  } else if (strcmp(req->command, "LIST_FREE_SLOTS") == 0) {
    return handle_list_free_slots(req, db_conn);
  }

  // MEETING COMMANDS
  else if (strcmp(req->command, "BOOK_INDIVIDUAL") == 0) {
    return handle_book_individual(req, db_conn);
  } else if (strcmp(req->command, "BOOK_GROUP") == 0) {
    return handle_book_group(req, db_conn);
  } else if (strcmp(req->command, "CANCEL_MEETING") == 0) {
    return handle_cancel_meeting(req, db_conn);
  } else if (strcmp(req->command, "LIST_MEETINGS") == 0) {
    return handle_list_meetings(req, db_conn);
  } else if (strcmp(req->command, "LIST_APPOINTMENTS") == 0) {
    return handle_list_appointments(req, db_conn);
  } else if (strcmp(req->command, "ADD_MINUTES") == 0) {
    return handle_add_minutes(req, db_conn);
  } else if (strcmp(req->command, "GET_MINUTES") == 0) {
    return handle_get_minutes(req, db_conn);
  } else if (strcmp(req->command, "VIEW_HISTORY") == 0) {
    return handle_view_history(req, db_conn);
  } else if (strcmp(req->command, "LIST_MY_SLOTS") == 0) {
    return handle_list_my_slots(req, db_conn);
  } else if (strcmp(req->command, "LIST_STUDENTS") == 0) {
    return handle_list_students(req, db_conn);
  } else if (strcmp(req->command, "LIST_ALL_STUDENTS") == 0) {
    return handle_list_all_students(req, db_conn);
  }

  // UNKNOWN COMMAND
  else {
    res->status_code = STATUS_BAD_REQUEST;
    snprintf(res->payload, sizeof(res->payload), "UNKNOWN_COMMAND: %s",
             req->command);
    return res;
  }
}

void handle_client(int client_fd, MYSQL *db_conn) {
  char buffer[BUFFER_SIZE];

  log_message("INFO", "Handling client: fd=%d", client_fd);

  int flag = 1;
  setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));

  while (1) {
    memset(buffer, 0, BUFFER_SIZE);
    int bytes = read_line(client_fd, buffer, BUFFER_SIZE);

    if (bytes <= 0) {
      log_message("INFO", "Client disconnected: fd=%d", client_fd);
      break;
    }

    if (bytes <= 2)
      continue;

    log_message("RECV", "%s", buffer);

    Request *req = parse_request(buffer);
    if (!req) {
      char *err_response = build_response(STATUS_BAD_REQUEST, "INVALID_FORMAT");
      write(client_fd, err_response, strlen(err_response));
      fsync(client_fd);
      log_message("SEND", "%s", err_response);
      free_response_string(err_response);
      continue;
    }

    Response *res = process_command(req, db_conn);

    char *response_msg = build_response(res->status_code, res->payload);
    size_t response_len = strlen(response_msg);

    ssize_t total_sent = 0;
    while (total_sent < (ssize_t)response_len) {
      ssize_t sent = write(client_fd, response_msg + total_sent,
                           response_len - total_sent);
      if (sent < 0)
        break;
      total_sent += sent;
    }

    if (total_sent > 0)
      fsync(client_fd);

    log_message("SEND", "%s", response_msg);

    free_response_string(response_msg);
    free(res);
    free_request(req);
  }

  usleep(10000);
  shutdown(client_fd, SHUT_RDWR);
  close(client_fd);

  log_message("INFO", "Client handler finished: fd=%d", client_fd);
}

// ============= MAIN =============
int main(void) {
  signal(SIGCHLD, SIG_IGN);

  log_message("INFO", "Starting Meeting Server on port %d", SERVER_PORT);

  MYSQL *db_conn = db_connect();
  if (!db_conn) {
    log_message("FATAL", "Cannot connect to database");
    return 1;
  }

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    log_message("FATAL", "Socket creation failed");
    return 1;
  }

  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(SERVER_PORT);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    log_message("FATAL", "Bind failed: %s", strerror(errno));
    return 1;
  }

  if (listen(server_fd, MAX_CLIENTS) < 0) {
    log_message("FATAL", "Listen failed");
    return 1;
  }

  log_message("INFO", "Server listening on port %d", SERVER_PORT);

  while (1) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_fd =
        accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

    int flag = 1;
    setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
    setsockopt(client_fd, SOL_SOCKET, SO_SNDBUF, &flag, sizeof(flag));

    if (client_fd < 0) {
      log_message("ERROR", "Accept failed");
      continue;
    }

    pid_t pid = fork();
    int client_id = ++client_counter;
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

    log_message("INFO", "Client #%d connected from %s (fd=%d)", client_id,
                client_ip, client_fd);

    if (pid == 0) {
      close(server_fd);

      MYSQL *child_db = db_connect();
      if (child_db) {
        handle_client(client_fd, child_db);
        db_close(child_db);
      }

      exit(0);
    } else if (pid > 0) {
      close(client_fd);
    } else {
      log_message("ERROR", "Fork failed");
    }
  }

  db_close(db_conn);
  close(server_fd);
  return 0;
}