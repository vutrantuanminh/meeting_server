#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

int connect_to_server(const char* host, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return -1;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sockfd);
        return -1;
    }
    
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

void close_connection(int sockfd) {
    if (sockfd >= 0) {
        shutdown(sockfd, SHUT_RDWR);
        close(sockfd);
    }
}

int send_request(int sockfd, const char* command, const char* token, const char* data) {
    char buffer[BUFFER_SIZE];
    
    // Format: COMMAND||TOKEN||DATA\r\n
    snprintf(buffer, sizeof(buffer), "%s||%s||%s\r\n", 
             command, 
             token ? token : "", 
             data ? data : "");
    
    ssize_t sent = write(sockfd, buffer, strlen(buffer));
    if (sent < 0) {
        perror("Send failed");
        return -1;
    }
    
    return 0;
}

char* receive_response(int sockfd) {
    static char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    
    int total = 0;
    char c;
    
    // Read until \r\n
    while (total < BUFFER_SIZE - 1) {
        int n = read(sockfd, &c, 1);
        
        if (n <= 0) {
            if (total == 0) return NULL;
            break;
        }
        
        buffer[total++] = c;
        
        // Check for \r\n
        if (total >= 2 && buffer[total-2] == '\r' && buffer[total-1] == '\n') {
            buffer[total] = '\0';
            return buffer;
        }
    }
    
    buffer[total] = '\0';
    return total > 0 ? buffer : NULL;
}
