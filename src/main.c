#define _CRT_SECURE_NO_WARNINGS
#include "server.h"
#include "config.h"
#include "logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

extern server_config_t g_cfg;
static volatile int g_running = 1;

void handle_signal(int sig) {
    printf("\n[DEBUG] Caught signal %d — stopping server...\n", sig);
    g_running = 0;
    server_stop();
}

int main(void) {
    printf("[DEBUG] main() started!\n");

    // Step 1: Load configuration
    printf("[DEBUG] Trying to load config file: ./config/server.conf\n");
    if (!load_config("config\\server.conf")) {
        printf("[ERROR] Failed to load configuration file.\n");
        return 1;
    }
    printf("[DEBUG] Configuration loaded successfully!\n");

    // Step 2: Initialize logger
    if (!log_init(g_cfg.log_file)) {
        printf("[ERROR] Failed to initialize logger: %s\n", g_cfg.log_file);
        return 1;
    }
    printf("[DEBUG] Logger initialized: %s\n", g_cfg.log_file);

    // Step 3: Handle Ctrl+C
    signal(SIGINT, handle_signal);

    // Step 4: Start server
    printf("[DEBUG] Starting the server on [%s]:%d...\n", g_cfg.listen_addr, g_cfg.listen_port);
    if (!server_start(g_cfg.listen_addr, g_cfg.listen_port)) {
        printf("[ERROR] server_start() failed!\n");
        log_close();
        return 1;
    }

    printf("[DEBUG] Server running — press Ctrl+C to stop.\n");

    // Step 5: Main loop
    while (g_running) {
        Sleep(1000);
    }

    // Step 6: Shutdown
    printf("[DEBUG] Shutting down server...\n");
    server_stop();
    log_close();
    printf("[DEBUG] Server stopped.\n");

    return 0;
}
