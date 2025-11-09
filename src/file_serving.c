#define _CRT_SECURE_NO_WARNINGS
#include "file_serving.h"
#include "logging.h"
#include "cache.h"
#include "config.h"
#include <windows.h>
#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

extern server_config_t g_cfg;

/* map extension to content-type (simple) */
static const char *mime_for(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";
    ext++;
    if (_stricmp(ext, "html") == 0 || _stricmp(ext, "htm") == 0) return "text/html; charset=utf-8";
    if (_stricmp(ext, "css") == 0) return "text/css";
    if (_stricmp(ext, "js") == 0) return "application/javascript";
    if (_stricmp(ext, "png") == 0) return "image/png";
    if (_stricmp(ext, "jpg") == 0 || _stricmp(ext, "jpeg") == 0) return "image/jpeg";
    if (_stricmp(ext, "gif") == 0) return "image/gif";
    if (_stricmp(ext, "svg") == 0) return "image/svg+xml";
    if (_stricmp(ext, "json") == 0) return "application/json";
    return "application/octet-stream";
}

/* send headers then use TransmitFile for zero-copy */
void serve_static_file(SOCKET client, const char *doc_root, const char *relpath) {
    char fullpath[MAX_PATH];
    snprintf(fullpath, sizeof(fullpath), "%s%s", doc_root, relpath);
    /* try cache first */
    cache_entry_t *ce = cache_get(fullpath);
    if (ce) {
        char hdr[512];
        int n = snprintf(hdr, sizeof(hdr),
            "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n",
            mime_for(fullpath), ce->len);
        send(client, hdr, n, 0);
        send(client, ce->data, (int)ce->len, 0);
        cache_release(ce);
        return;
    }

    /* open file using CreateFile to use TransmitFile */
    HANDLE fh = CreateFileA(fullpath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fh == INVALID_HANDLE_VALUE) {
        log_debug("file not found: %s", fullpath);
        const char *resp = "HTTP/1.1 404 Not Found\r\nContent-Length: 13\r\nConnection: close\r\n\r\n404 Not Found";
        send(client, resp, (int)strlen(resp), 0);
        return;
    }

    LARGE_INTEGER filesize_li;
    if (!GetFileSizeEx(fh, &filesize_li)) {
        CloseHandle(fh);
        const char *resp = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 21\r\nConnection: close\r\n\r\nInternal Server Error";
        send(client, resp, (int)strlen(resp), 0);
        return;
    }
    SIZE_T filesize = (SIZE_T)filesize_li.QuadPart;

    /* optional caching for small files */
    if (filesize < 64 * 1024 && g_cfg.cache_max_entries > 0) {
        /* read into memory and put into cache */
        char *buf = (char *)malloc(filesize);
        DWORD readbytes = 0;
        BOOL rr = ReadFile(fh, buf, (DWORD)filesize, &readbytes, NULL);
        if (!rr || readbytes != filesize) {
            free(buf);
            CloseHandle(fh);
            const char *resp = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 21\r\nConnection: close\r\n\r\nInternal Server Error";
            send(client, resp, (int)strlen(resp), 0);
            return;
        }
        cache_put(fullpath, buf, readbytes);
        /* send via simple send (it's small) */
        char hdr[512];
        int n = snprintf(hdr, sizeof(hdr),
            "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n",
            mime_for(fullpath), (size_t)readbytes);
        send(client, hdr, n, 0);
        send(client, buf, (int)readbytes, 0);
        free(buf);
        CloseHandle(fh);
        return;
    }

    /* send headers, then TransmitFile */
    char hdr[512];
    int hlen = snprintf(hdr, sizeof(hdr),
        "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n",
        mime_for(fullpath), (size_t)filesize);
    send(client, hdr, hlen, 0);

    /* TransmitFile needs a HANDLE cast to SOCKET via TransmitFile API */
    TRANSMIT_FILE_BUFFERS tfb;
    memset(&tfb, 0, sizeof(tfb));
    /* TransmitFile returns nonzero for success */
    BOOL ok = TransmitFile((SOCKET)client, fh, 0, 0, NULL, &tfb, 0);
    if (!ok) {
        log_error("TransmitFile failed for %s (err %lu)", fullpath, GetLastError());
    }
    CloseHandle(fh);
}
