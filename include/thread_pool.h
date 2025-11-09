#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#include "utils.h"

/* job: client socket + client addr info */
typedef struct {
    SOCKET client;
    struct sockaddr_in6 addr6; /* supports IPv4/IPv6 mapped */
    int addr_len;
} job_t;

typedef struct thread_pool thread_pool_t;

thread_pool_t *thread_pool_create(int num_threads, int queue_size);
void thread_pool_destroy(thread_pool_t *tp);
int thread_pool_enqueue(thread_pool_t *tp, job_t job);
void thread_pool_shutdown(thread_pool_t *tp);

#endif
