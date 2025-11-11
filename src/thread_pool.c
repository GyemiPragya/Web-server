#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <winsock2.h>
#include "thread_pool.h"
#include "logging.h"
#include "http_handler.h"

/* --- Job structure ---
typedef struct {
    SOCKET client_sock;
    const char *client_ip;
    // Add other job-related fields if needed
} job_t;
 */

// --- Thread pool structure ---
struct thread_pool {
    thread_t *threads;
    int num_threads;
    job_t *queue;
    int queue_size;
    int head, tail, count;
    mutex_t lock;
    condvar_t not_empty;
    condvar_t not_full;
    int shutting_down;
};

// --- Worker thread function ---
static DWORD WINAPI worker_thread(LPVOID arg) {
    thread_pool_t *tp = (thread_pool_t *)arg;
    while (1) {
        mutex_lock(&tp->lock);
        while (tp->count == 0 && !tp->shutting_down) {
            cond_wait(&tp->not_empty, &tp->lock);
        }
        if (tp->shutting_down && tp->count == 0) {
            mutex_unlock(&tp->lock);
            break;
        }
        job_t job = tp->queue[tp->head];
        tp->head = (tp->head + 1) % tp->queue_size;
        tp->count--;
        cond_signal(&tp->not_full);
        mutex_unlock(&tp->lock);

        process_job(job.client_sock, job.client_ip);
    }
    return 0;
}

// --- Create the thread pool ---
thread_pool_t *thread_pool_create(int num_threads, int queue_size) {
    thread_pool_t *tp = (thread_pool_t *)malloc(sizeof(thread_pool_t));
    if (!tp) return NULL;
    tp->num_threads = num_threads;
    tp->queue_size = queue_size;
    tp->queue = (job_t *)malloc(sizeof(job_t) * queue_size);
    tp->head = tp->tail = tp->count = 0;
    mutex_init(&tp->lock);
    cond_init(&tp->not_empty);
    cond_init(&tp->not_full);
    tp->shutting_down = 0;
    tp->threads = (thread_t *)malloc(sizeof(thread_t) * num_threads);
    for (int i = 0; i < num_threads; ++i) {
        tp->threads[i] = thread_create(worker_thread, tp);
        thread_detach(tp->threads[i]);
    }
    log_info("Thread pool created with %d threads, queue size %d", num_threads, queue_size);
    return tp;
}

// --- Destroy the thread pool ---
void thread_pool_destroy(thread_pool_t *tp) {
    if (!tp) return;
    mutex_lock(&tp->lock);
    tp->shutting_down = 1;
    cond_broadcast(&tp->not_empty);
    mutex_unlock(&tp->lock);
    Sleep(100);
    free(tp->threads);
    free(tp->queue);
    mutex_destroy(&tp->lock);
    free(tp);
}

// --- Enqueue a job into the thread pool ---
int thread_pool_enqueue(thread_pool_t *tp, job_t job) {
    mutex_lock(&tp->lock);
    while (tp->count == tp->queue_size && !tp->shutting_down) {
        cond_wait(&tp->not_full, &tp->lock);
    }
    if (tp->shutting_down) {
        mutex_unlock(&tp->lock);
        return -1;
    }
    tp->queue[tp->tail] = job;
    tp->tail = (tp->tail + 1) % tp->queue_size;
    tp->count++;
    cond_signal(&tp->not_empty);
    mutex_unlock(&tp->lock);
    return 0;
}

// --- Gracefully shut down the thread pool ---
void thread_pool_shutdown(thread_pool_t *tp) {
    if (!tp) return;
    mutex_lock(&tp->lock);
    tp->shutting_down = 1;
    cond_broadcast(&tp->not_empty);
    mutex_unlock(&tp->lock);
}
