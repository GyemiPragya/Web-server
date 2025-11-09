#ifndef SERVER_H
#define SERVER_H

#include "config.h"   // we already defined server_config_t there

// --- Server lifecycle ---
int server_start(const char *addr, int port);
void server_stop(void);

#endif
