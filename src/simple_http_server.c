#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // For close()
#include <arpa/inet.h>  // For socket functions
#include <netinet/in.h>
#include <sys/socket.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Function to send HTTP response
void send_response(int client_sock, const char *status, const char *body) {
    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response),
             "HTTP/1.1 %s\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             status, strlen(body), body);
    send(client_sock, response, strlen(response), 0);
}

// Function to handle each client connection
void handle_client(int client_sock) {
    char buffer[BUFFER_SIZE];
    int bytes_read = read(client_sock, buffer, sizeof(buffer) - 1);
    if (bytes_read < 1) {
        close(client_sock);
        return;
    }
    buffer[bytes_read] = '\0';  // null-terminate

    // Basic parsing of request line
    char method[8], path[256], version[16];
    sscanf(buffer, "%s %s %s", method, path, version);

    printf("[INFO] Received request: %s %s\n", method, path);

    // Basic routing
    if (strcmp(method, "GET") == 0) {
        if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
            const char *body = "<h1>Welcome to the Page Replacement Visualizer</h1>";
            send_response(client_sock, "200 OK", body);
        } else if (strcmp(path, "/about") == 0) {
            const char *body = "<h1>About this OS Page Replacement Visualizer</h1>";
            send_response(client_sock, "200 OK", body);
        } else {
            const char *body = "<h1>404 Not Found</h1>";
            send_response(client_sock, "404 Not Found", body);
        }
    } else {
        const char *body = "<h1>501 Not Implemented</h1>";
        send_response(client_sock, "501 Not Implemented", body);
    }

    close(client_sock);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;

    // Create socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket");
        exit(EXIT_FAILURE);
    }

    // Bind socket
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_sock, 10) == -1) {
        perror("Listen");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("[INFO] Server listening on port %d...\n", PORT);

    while (1) {
        client_addr_len = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_sock == -1) {
            perror("Accept");
            continue;
        }

        handle_client(client_sock);
    }

    close(server_sock);
    return 0;
}
