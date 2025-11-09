#define _CRT_SECURE_NO_WARNINGS
#include "ddos_protection.h"
#include "utils.h"
#include "logging.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TABLE_SIZE 4096

typedef struct ip_node {
    char ip[64];
    int tokens;
    int capacity;
    int refill_per_sec;
    long last_refill;
    int connections;
    long banned_until;
    struct ip_node *next;
} ip_node_t;

static ip_node_t *table[TABLE_SIZE];
static mutex_t ddos_mutex;
static int global_capacity = 40;
static int global_refill = 10;
static int global_ban = 60;
static int global_max_conn = 50;

static unsigned long hstr(const char *s) {
    unsigned long h = 5381;
    while (*s) h = ((h << 5) + h) + (unsigned char)*s++;
    return h;
}

int ddos_init(int capacity, int refill_per_sec, int ban_duration_sec, int max_conn_per_ip) {
    global_capacity = capacity;
    global_refill = refill_per_sec;
    global_ban = ban_duration_sec;
    global_max_conn = max_conn_per_ip;
    memset(table, 0, sizeof(table));
    mutex_init(&ddos_mutex);
    return 0;
}

static ip_node_t *get_or_create(const char *ip) {
    unsigned long idx = hstr(ip) % TABLE_SIZE;
    ip_node_t *n = table[idx];
    while (n) {
        if (strcmp(n->ip, ip) == 0) return n;
        n = n->next;
    }
    n = (ip_node_t *)malloc(sizeof(ip_node_t));
    memset(n, 0, sizeof(*n));
    strcpy(n->ip, ip);
    n->capacity = global_capacity;
    n->tokens = n->capacity;
    n->refill_per_sec = global_refill;
    n->last_refill = time(NULL);
    n->connections = 0;
    n->banned_until = 0;
    n->next = table[idx];
    table[idx] = n;
    return n;
}

int ddos_check_and_consume(const char *ipstr) {
    mutex_lock(&ddos_mutex);
    ip_node_t *n = get_or_create(ipstr);
    long now = time(NULL);
    if (n->banned_until > now) {
        mutex_unlock(&ddos_mutex);
        log_info("IP %s is currently banned until %ld", ipstr, n->banned_until);
        return -2;
    }
    long diff = now - n->last_refill;
    if (diff > 0) {
        long add = diff * n->refill_per_sec;
        n->tokens += (int)add;
        if (n->tokens > n->capacity) n->tokens = n->capacity;
        n->last_refill = now;
    }
    if (n->tokens <= 0) {
        n->banned_until = now + global_ban;
        mutex_unlock(&ddos_mutex);
        log_info("IP %s banned for %d seconds (rate limit)", ipstr, global_ban);
        return -1;
    }
    if (n->connections >= global_max_conn) {
        mutex_unlock(&ddos_mutex);
        log_info("IP %s reached max connections %d", ipstr, global_max_conn);
        return -3;
    }
    n->tokens--;
    n->connections++;
    mutex_unlock(&ddos_mutex);
    return 0;
}

void ddos_release_connection(const char *ipstr) {
    mutex_lock(&ddos_mutex);
    unsigned long idx = hstr(ipstr) % TABLE_SIZE;
    ip_node_t *n = table[idx];
    while (n) {
        if (strcmp(n->ip, ipstr) == 0) {
            if (n->connections > 0) n->connections--;
            break;
        }
        n = n->next;
    }
    mutex_unlock(&ddos_mutex);
}

void ddos_shutdown(void) {
    mutex_lock(&ddos_mutex);
    for (int i = 0; i < TABLE_SIZE; ++i) {
        ip_node_t *n = table[i];
        while (n) {
            ip_node_t *nx = n->next;
            free(n);
            n = nx;
        }
        table[i] = NULL;
    }
    mutex_unlock(&ddos_mutex);
    mutex_destroy(&ddos_mutex);
}
