#ifndef SERVER_H
#define SERVER_H

#include <mysql/mysql.h>
#include "protocol.h"

#define SERVER_PORT 1234
#define MAX_CLIENTS 100
#define BUFFER_SIZE 8192

// Handle client connection
void handle_client(int client_fd, MYSQL* db_conn);

// Read one line ending with \r\n
int read_line(int fd, char* buffer, int max_len);

// Process command và trả về response
Response* process_command(Request* req, MYSQL* db_conn);

#endif
