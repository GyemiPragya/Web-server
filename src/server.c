#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include "server.h"
#include "logging.h"
#include "config.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

static SOCKET listen_sock = INVALID_SOCKET;
static int running = 0;

void server_stop(void) {
    if (listen_sock != INVALID_SOCKET) {
        closesocket(listen_sock);
        listen_sock = INVALID_SOCKET;
    }
    WSACleanup();
    running = 0;
    printf("[INFO] Server stopped.\n");
}

int server_start(const char *addr, int port) {
    WSADATA wsa;
    struct sockaddr_in6 server_addr;

    printf("[DEBUG] Initializing Winsock...\n");
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("[ERROR] WSAStartup failed.\n");
        return 0;
    }

    listen_sock = socket(AF_INET6, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET) {
        printf("[ERROR] Failed to create socket.\n");
        WSACleanup();
        return 0;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(port);
    inet_pton(AF_INET6, addr, &server_addr.sin6_addr);

    if (bind(listen_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("[ERROR] Bind failed with error %d\n", WSAGetLastError());
        closesocket(listen_sock);
        WSACleanup();
        return 0;
    }

    if (listen(listen_sock, 10) == SOCKET_ERROR) {
        printf("[ERROR] Listen failed with error %d\n", WSAGetLastError());
        closesocket(listen_sock);
        WSACleanup();
        return 0;
    }

    printf("[INFO] Listening on port %d...\n", port);
    running = 1;

    // Minimal server loop (accepts and immediately closes connections)
    while (running) {
        SOCKET client = accept(listen_sock, NULL, NULL);
        if (client != INVALID_SOCKET) {
            printf("[DEBUG] Accepted a connection.\n");
            closesocket(client);
        }
    }

    server_stop();
    return 1;
}
