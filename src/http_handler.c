#define _CRT_SECURE_NO_WARNINGS
#include "http_handler.h"
#include "file_serving.h"
#include "logging.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <winsock2.h>
#include <windows.h>

// --- Metrics counters (global) ---
int metric_requests_total = 0;
int metric_active_connections = 0;

// --- Helper: send HTTP response ---
static void send_response(SOCKET client_sock, const char *status, const char *content_type, const char *body) {
    char response[2048];
    int len = snprintf(response, sizeof(response),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n%s",
        status, content_type, strlen(body), body);

    send(client_sock, response, len, 0);
}

// --- Process a single client request ---
void process_job(SOCKET client_sock, const char *client_ip) {
    char buffer[1024];
    int bytes_received;

    metric_active_connections++;
    metric_requests_total++;

    bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        closesocket(client_sock);
        metric_active_connections--;
        return;
    }

    buffer[bytes_received] = '\0';
    log_info("Received request:\n%s", buffer);

    // Simple check: if it's a GET /
    if (strncmp(buffer, "GET / ", 6) == 0) {
        send_response(client_sock, "200 OK", "text/html", "<h1>Welcome to Prachi’s Web Server!</h1>");
    } else if (strncmp(buffer, "GET /metrics", 12) == 0) {
        char metrics[256];
        snprintf(metrics, sizeof(metrics),
            "requests_total %d\nactive_connections %d\n",
            metric_requests_total, metric_active_connections);
        send_response(client_sock, "200 OK", "text/plain", metrics);
    } else {
        send_response(client_sock, "404 Not Found", "text/plain", "Not Found");
    }

    closesocket(client_sock);
    metric_active_connections--;
}
