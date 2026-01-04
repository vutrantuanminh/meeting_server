#ifndef NETWORK_H
#define NETWORK_H

#define SERVER_HOST "127.0.0.1"
#define SERVER_PORT 1234
#define BUFFER_SIZE 8192

// Connection management
int connect_to_server(const char* host, int port);
void close_connection(int sockfd);

// Communication
int send_request(int sockfd, const char* command, const char* token, const char* data);
char* receive_response(int sockfd);

#endif
