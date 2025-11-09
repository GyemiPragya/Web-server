#ifndef CACHE_H
#define CACHE_H
#include <stddef.h>

typedef struct cache_entry {
    char *path;
    char *data;
    size_t len;
    struct cache_entry *prev, *next;
} cache_entry_t;

int cache_init(int max_entries);
void cache_destroy(void);
cache_entry_t *cache_get(const char *path);
void cache_put(const char *path, const char *data, size_t len);
void cache_release(cache_entry_t *e);
int cache_count(void);

#endif
