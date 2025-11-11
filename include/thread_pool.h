#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#include "utils.h"

#include <winsock2.h> // ensure SOCKET is defined

typedef struct {
    SOCKET client_sock;
    const char *client_ip;
    // Add other job fields if needed
} job_t;


typedef struct thread_pool thread_pool_t;

thread_pool_t *thread_pool_create(int num_threads, int queue_size);
void thread_pool_destroy(thread_pool_t *tp);
int thread_pool_enqueue(thread_pool_t *tp, job_t job);
void thread_pool_shutdown(thread_pool_t *tp);

#endif
