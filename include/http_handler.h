// #ifndef HTTP_HANDLER_H
// #define HTTP_HANDLER_H
// #include "thread_pool.h"

// void process_job(job_t *job);

// #endif


// --- in include/http_handler.h ---
#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

#include <winsock2.h>

void process_job(SOCKET client_sock, const char *client_ip);

#endif
