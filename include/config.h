#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    char listen_addr[64];
    int listen_port;
    char document_root[256];
    int num_threads;
    int queue_size;
    int max_connections_per_ip;
    int rate_limit_tokens;
    int rate_limit_refill_per_sec;
    int ban_duration_sec;
    int connection_timeout_sec;
    int keepalive_timeout_sec;
    int cache_max_entries;
    char log_file[256];
    int enable_metrics;
    char metrics_path[128];
    char status_path[128];
} server_config_t;

extern server_config_t g_cfg;

int load_config(const char *filename);

#endif
