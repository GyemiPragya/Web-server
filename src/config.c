#define _CRT_SECURE_NO_WARNINGS
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global configuration object
server_config_t g_cfg;

int load_config(const char *filename) {
    printf("[DEBUG] load_config(): trying to open '%s'\n", filename);

    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("[ERROR] fopen");
        return 0;
    }

    printf("[DEBUG] Config file opened successfully!\n");

    // Initialize defaults
    strcpy(g_cfg.listen_addr, "0.0.0.0");
    g_cfg.listen_port = 8000;
    strcpy(g_cfg.document_root, "./www");
    g_cfg.num_threads = 4;
    g_cfg.queue_size = 128;
    g_cfg.max_connections_per_ip = 20;
    g_cfg.rate_limit_tokens = 10;
    g_cfg.rate_limit_refill_per_sec = 5;
    g_cfg.ban_duration_sec = 60;
    g_cfg.connection_timeout_sec = 10;
    g_cfg.keepalive_timeout_sec = 5;
    g_cfg.cache_max_entries = 64;
    strcpy(g_cfg.log_file, "webserver.log");
    g_cfg.enable_metrics = 0;
    strcpy(g_cfg.metrics_path, "/metrics");
    strcpy(g_cfg.status_path, "/status");

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        // skip comments and blank lines
        if (line[0] == '#' || line[0] == '\n')
            continue;

        char key[128], value[256];
        if (sscanf(line, " %127[^=]= %255[^\n]", key, value) == 2) {
            // trim trailing whitespace/newlines
            for (char *p = value; *p; p++) {
                if (*p == '\r' || *p == '\n') *p = 0;
            }

            // parse each config field
            if (strcmp(key, "listen_addr") == 0)
                strcpy(g_cfg.listen_addr, value);
            else if (strcmp(key, "listen_port") == 0)
                g_cfg.listen_port = atoi(value);
            else if (strcmp(key, "document_root") == 0)
                strcpy(g_cfg.document_root, value);
            else if (strcmp(key, "num_threads") == 0)
                g_cfg.num_threads = atoi(value);
            else if (strcmp(key, "queue_size") == 0)
                g_cfg.queue_size = atoi(value);
            else if (strcmp(key, "max_connections_per_ip") == 0)
                g_cfg.max_connections_per_ip = atoi(value);
            else if (strcmp(key, "rate_limit_tokens") == 0)
                g_cfg.rate_limit_tokens = atoi(value);
            else if (strcmp(key, "rate_limit_refill_per_sec") == 0)
                g_cfg.rate_limit_refill_per_sec = atoi(value);
            else if (strcmp(key, "ban_duration_sec") == 0)
                g_cfg.ban_duration_sec = atoi(value);
            else if (strcmp(key, "connection_timeout_sec") == 0)
                g_cfg.connection_timeout_sec = atoi(value);
            else if (strcmp(key, "keepalive_timeout_sec") == 0)
                g_cfg.keepalive_timeout_sec = atoi(value);
            else if (strcmp(key, "cache_max_entries") == 0)
                g_cfg.cache_max_entries = atoi(value);
            else if (strcmp(key, "log_file") == 0)
                strcpy(g_cfg.log_file, value);
            else if (strcmp(key, "enable_metrics") == 0)
                g_cfg.enable_metrics = atoi(value);
            else if (strcmp(key, "metrics_path") == 0)
                strcpy(g_cfg.metrics_path, value);
            else if (strcmp(key, "status_path") == 0)
                strcpy(g_cfg.status_path, value);
        }
    }

    fclose(f);

    printf("[DEBUG] Final config loaded:\n");
    printf("  listen_addr = %s\n", g_cfg.listen_addr);
    printf("  listen_port = %d\n", g_cfg.listen_port);
    printf("  document_root = %s\n", g_cfg.document_root);
    printf("  log_file = %s\n", g_cfg.log_file);
    printf("  num_threads = %d\n", g_cfg.num_threads);
    printf("  cache_max_entries = %d\n", g_cfg.cache_max_entries);
    printf("  enable_metrics = %d\n", g_cfg.enable_metrics);
    printf("--------------------------------------\n");

    return 1;
}
