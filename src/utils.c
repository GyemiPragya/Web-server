#include "utils.h"
#include <stdio.h>
// Trim whitespace (in-place)
char *trim_whitespace(char *str) {
    char *end;

    // Empty string check
    if (str == NULL || *str == '\0')
        return str;

    // Skip leading spaces
    while (isspace((unsigned char)*str)) str++;

    if (*str == 0)  // All spaces
        return str;

    // Trim trailing spaces
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator
    *(end + 1) = '\0';

    return str;
}

// Convert to lowercase (in-place)
char *str_tolower(char *str) {
    for (char *p = str; *p; ++p)
        *p = (char)tolower(*p);
    return str;
}

// Check if a string starts with a prefix
int starts_with(const char *str, const char *prefix) {
    if (!str || !prefix) return 0;
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

// Return a formatted timestamp "YYYY-MM-DD HH:MM:SS"
void get_timestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", t);
}

// Safe malloc (exits on failure)
void *xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "xmalloc: Out of memory!\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

// Safe calloc (exits on failure)
void *xcalloc(size_t n, size_t size) {
    void *ptr = calloc(n, size);
    if (!ptr) {
        fprintf(stderr, "xcalloc: Out of memory!\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

// Safe strdup (exits on failure)
char *xstrdup(const char *s) {
    char *copy = _strdup(s); // _strdup is MSVC’s version
    if (!copy) {
        fprintf(stderr, "xstrdup: Out of memory!\n");
        exit(EXIT_FAILURE);
    }
    return copy;
}
