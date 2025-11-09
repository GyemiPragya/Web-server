#ifndef FILE_SERVING_H
#define FILE_SERVING_H
#include <winsock2.h>

void serve_static_file(SOCKET client, const char *doc_root, const char *relpath);

#endif
