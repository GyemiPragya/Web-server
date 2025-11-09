#define _CRT_SECURE_NO_WARNINGS
#include "cache.h"
#include "utils.h"
#include "logging.h"
#include <stdlib.h>
#include <string.h>

#define TABLE_SIZE 1024

static cache_entry_t *table[TABLE_SIZE];
static cache_entry_t *lru_head = NULL;
static cache_entry_t *lru_tail = NULL;
static int cur_entries = 0;
static int max_entries = 0;
static mutex_t cache_mutex;

static unsigned long hash_str(const char *s) {
    unsigned long h = 5381;
    while (*s) h = ((h << 5) + h) + (unsigned char)*s++;
    return h;
}

static void touch(cache_entry_t *e) {
    if (e == lru_head) return;
    if (e->prev) e->prev->next = e->next;
    if (e->next) e->next->prev = e->prev;
    if (e == lru_tail) lru_tail = e->prev;
    e->prev = NULL;
    e->next = lru_head;
    if (lru_head) lru_head->prev = e;
    lru_head = e;
    if (!lru_tail) lru_tail = e;
}

int cache_init(int max_e) {
    max_entries = max_e;
    for (int i = 0; i < TABLE_SIZE; ++i) table[i] = NULL;
    cur_entries = 0;
    mutex_init(&cache_mutex);
    return 0;
}

void cache_destroy(void) {
    mutex_lock(&cache_mutex);
    for (int i = 0; i < TABLE_SIZE; ++i) {
        cache_entry_t *e = table[i];
        while (e) {
            cache_entry_t *n = e->next;
            free(e->path);
            free(e->data);
            free(e);
            e = n;
        }
        table[i] = NULL;
    }
    lru_head = lru_tail = NULL;
    cur_entries = 0;
    mutex_unlock(&cache_mutex);
    mutex_destroy(&cache_mutex);
}

cache_entry_t *cache_get(const char *path) {
    mutex_lock(&cache_mutex);
    unsigned long h = hash_str(path) % TABLE_SIZE;
    cache_entry_t *e = table[h];
    while (e) {
        if (strcmp(e->path, path) == 0) {
            touch(e);
            mutex_unlock(&cache_mutex);
            return e;
        }
        e = e->next;
    }
    mutex_unlock(&cache_mutex);
    return NULL;
}

void cache_put(const char *path, const char *data, size_t len) {
    if (max_entries <= 0) return;
    mutex_lock(&cache_mutex);
    while (cur_entries >= max_entries && lru_tail) {
        cache_entry_t *ev = lru_tail;
        unsigned long h = hash_str(ev->path) % TABLE_SIZE;
        cache_entry_t **pp = &table[h];
        while (*pp && *pp != ev) pp = &(*pp)->next;
        if (*pp) *pp = ev->next;
        if (ev->prev) ev->prev->next = NULL;
        lru_tail = ev->prev;
        if (ev == lru_head) lru_head = NULL;
        free(ev->path); free(ev->data); free(ev);
        cur_entries--;
    }
    cache_entry_t *e = (cache_entry_t *)malloc(sizeof(cache_entry_t));
    e->path = _strdup(path);
    e->data = (char *)malloc(len);
    memcpy(e->data, data, len);
    e->len = len;
    e->prev = NULL;
    unsigned long h = hash_str(path) % TABLE_SIZE;
    e->next = table[h];
    table[h] = e;
    if (lru_head) lru_head->prev = e;
    e->next = lru_head;
    lru_head = e;
    if (!lru_tail) lru_tail = e;
    cur_entries++;
    mutex_unlock(&cache_mutex);
}

void cache_release(cache_entry_t *e) { (void)e; }

int cache_count(void) {
    mutex_lock(&cache_mutex);
    int c = cur_entries;
    mutex_unlock(&cache_mutex);
    return c;
}
