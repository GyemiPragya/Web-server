#ifndef LOGGING_H
#define LOGGING_H
#include <stdarg.h>

int log_init(const char *path);
void log_close(void);
void log_info(const char *fmt, ...);
void log_error(const char *fmt, ...);
void log_debug(const char *fmt, ...);

#endif
