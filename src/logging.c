#define _CRT_SECURE_NO_WARNINGS
#include "logging.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "utils.h"

static FILE *log_fp = NULL;
static mutex_t log_mutex;

int log_init(const char *path) {
    mutex_init(&log_mutex);
    if (path == NULL) return -1;
    log_fp = fopen(path, "a");
    if (!log_fp) {
        fprintf(stderr, "[ERROR] Could not open log file at '%s': ", path);
        perror("");
        return -1;
    }
    setvbuf(log_fp, NULL, _IOLBF, 0);
    return 0;
}


void log_close(void) {
    mutex_lock(&log_mutex);
    if (log_fp) { fclose(log_fp); log_fp = NULL; }
    mutex_unlock(&log_mutex);
    mutex_destroy(&log_mutex);
}

static void log_v(const char *level, const char *fmt, va_list ap) {
    mutex_lock(&log_mutex);
    if (!log_fp) log_fp = stderr;
    time_t t = time(NULL);
    struct tm tm;
    localtime_s(&tm, &t);
    char ts[64];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tm);
    fprintf(log_fp, "[%s] %s: ", ts, level);
    vfprintf(log_fp, fmt, ap);
    fprintf(log_fp, "\n");
    fflush(log_fp);
    mutex_unlock(&log_mutex);
}

void log_info(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); log_v("INFO", fmt, ap); va_end(ap);
}
void log_error(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); log_v("ERROR", fmt, ap); va_end(ap);
}
void log_debug(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); log_v("DEBUG", fmt, ap); va_end(ap);
}
