#define _CRT_SECURE_NO_WARNINGS
#include <ws2tcpip.h>
#include <winsock2.h>
#include <process.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <windows.h>
#include "page_replacement.h"

#pragma comment(lib, "Ws2_32.lib")

volatile int running = 1;

// Global metrics for dashboard
volatile long total_requests = 0;
volatile long total_threads_created = 0;
volatile long active_connections = 0;

int parse_csv(const char *s, int *arr, int max) {
    int n = 0, val = 0, neg = 0;
    while (*s && n < max) {
        if (isdigit(*s) || (*s == '-' && isdigit(*(s+1)))) {
            neg = 0;
            if (*s == '-') { neg = 1; ++s; }
            val = 0;
            while (isdigit(*s)) { val = val*10 + (*s - '0'); ++s; }
            arr[n++] = neg ? -val : val;
        }
        while (*s && !isdigit(*s) && *s != '-') ++s;
    }
    return n;
}

void parse_query(const char *req, char *out_refs, char *out_frames, char *out_algo) {
    const char *get = strstr(req, "GET /api/visualize?");
    if (get) {
        const char *p = get + strlen("GET /api/visualize?");
        const char *end = strchr(p, ' ');
        char query[512]; 
        memset(query, 0, 512);
        if (end) strncpy(query, p, end-p);
        else strcpy(query, p);
        
        char *tok = strtok(query, "&");
        while (tok) {
            if (strncmp(tok, "refs=", 5) == 0)
                strcpy(out_refs, tok + 5);
            if (strncmp(tok, "frames=", 7) == 0)
                strcpy(out_frames, tok + 7);
            if (strncmp(tok, "algo=", 5) == 0)
                strcpy(out_algo, tok + 5);
            tok = strtok(NULL, "&");
        }
    }
}

void format_json_response(char *buf, int bufsize, FrameState *steps, int step_count) {
    int off = 0;
    off += snprintf(buf + off, bufsize - off, "{\"steps\":[");
    
    for (int i = 0; i < step_count; i++) {
        off += snprintf(buf + off, bufsize - off, 
            "{\"ref\":%d,\"fault\":%d,\"replaced\":%d,\"frames\":[",
            steps[i].ref_page, steps[i].fault, steps[i].replaced);
        
        for (int j = 0; j < steps[i].num_frames; j++) {
            off += snprintf(buf + off, bufsize - off, "%d", steps[i].frames[j]);
            if (j < steps[i].num_frames - 1) 
                off += snprintf(buf + off, bufsize - off, ",");
        }
        off += snprintf(buf + off, bufsize - off, "]}");
        if (i < step_count - 1)
            off += snprintf(buf + off, bufsize - off, ",");
    }
    off += snprintf(buf + off, bufsize - off, "]}");
}

unsigned __stdcall worker_thread(void *lp) {
    SOCKET c = (SOCKET)lp;
    InterlockedIncrement(&active_connections);
    InterlockedIncrement(&total_requests);
    
    char req[2048] = {0};
    recv(c, req, 2047, 0);
    
    char refs_str[256] = {0}, frames_str[64] = {0}, algo[64] = {0};

    // Handle API endpoint for visualization data
    if (strstr(req, "GET /api/visualize?")) {
        parse_query(req, refs_str, frames_str, algo);
        
        int refs[256], num_refs = parse_csv(refs_str, refs, 256);
        int num_frames = atoi(frames_str);
        
        if (num_frames < 1) num_frames = 3;
        
        char respbuf[16384];
        if (num_refs > 0) {
            FrameState *steps = NULL;
            int step_count = 0;
            
            if (strcmp(algo, "fifo") == 0) {
                steps = simulate_fifo(refs, num_refs, num_frames, &step_count);
            } else if (strcmp(algo, "lru") == 0) {
                steps = simulate_lru(refs, num_refs, num_frames, &step_count);
            } else if (strcmp(algo, "optimal") == 0) {
                steps = simulate_optimal(refs, num_refs, num_frames, &step_count);
            } else {
                steps = simulate_fifo(refs, num_refs, num_frames, &step_count);
            }
            
            char json[8192];
            format_json_response(json, sizeof(json), steps, step_count);
            
            snprintf(respbuf, sizeof(respbuf),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Connection: close\r\n"
                "\r\n%s", json);
            
            free_frame_states(steps, step_count);
        } else {
            snprintf(respbuf, sizeof(respbuf),
                "HTTP/1.1 400 Bad Request\r\n"
                "Content-Type: application/json\r\n"
                "Connection: close\r\n"
                "\r\n{\"error\":\"Invalid input\"}");
        }
        send(c, respbuf, (int)strlen(respbuf), 0);
    }
    // Handle metrics endpoint for dashboard
    else if (strstr(req, "GET /api/metrics")) {
        char metrics[512];
        snprintf(metrics, sizeof(metrics),
            "{"
            "\"total_requests\":%ld,"
            "\"total_threads\":%ld,"
            "\"active_connections\":%ld"
            "}",
            total_requests, total_threads_created, active_connections);
        
        char respbuf[1024];
        snprintf(respbuf, sizeof(respbuf),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Connection: close\r\n"
            "\r\n%s", metrics);
        send(c, respbuf, (int)strlen(respbuf), 0);
    }
    else {
        // Serve static HTML files
        char filepath[512];
        char method[16], path[256], version[16];
        sscanf(req, "%s %s %s", method, path, version);
        
        if (strcmp(path, "/") == 0) {
            strcpy(path, "/index.html");
        }
        
        snprintf(filepath, sizeof(filepath), "../www%s", path);
        
        FILE *file = fopen(filepath, "rb");
        if (file) {
            fseek(file, 0, SEEK_END);
            long filesize = ftell(file);
            fseek(file, 0, SEEK_SET);
            
            char *content = (char*)malloc(filesize + 1);
            fread(content, 1, filesize, file);
            content[filesize] = '\0';
            fclose(file);
            
            const char *content_type = "text/html";
            if (strstr(path, ".css")) content_type = "text/css";
            else if (strstr(path, ".js")) content_type = "application/javascript";
            else if (strstr(path, ".json")) content_type = "application/json";
            
            char header[512];
            int hlen = snprintf(header, sizeof(header),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: %s\r\n"
                "Content-Length: %ld\r\n"
                "Connection: close\r\n"
                "\r\n", content_type, filesize);
            
            send(c, header, hlen, 0);
            send(c, content, (int)filesize, 0);
            free(content);
        } else {
            const char *resp = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n<h1>404 Not Found</h1>";
            send(c, resp, (int)strlen(resp), 0);
        }
    }
    
    closesocket(c);
    InterlockedDecrement(&active_connections);
    return 0;
}

int server_start(const char *addr, int port) {
    WSADATA wsa;
    struct sockaddr_in server_addr;
    printf("[DEBUG] Initializing Winsock...\n");
    if (WSAStartup(MAKEWORD(2,2), &wsa)) return 0;

    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, addr, &server_addr.sin_addr);

    if (bind(listen_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR
        || listen(listen_sock, 8) == SOCKET_ERROR) return 0;

    printf("Serving on %s:%d\n", addr, port);
    printf("Open http://localhost:%d in your browser\n", port);
    running = 1;
    while (running) {
        SOCKET c = accept(listen_sock, NULL, NULL);
        if (c != INVALID_SOCKET) {
            InterlockedIncrement(&total_threads_created);
            _beginthreadex(NULL, 0, worker_thread, (void*)c, 0, NULL);
        }
    }
    closesocket(listen_sock);
    WSACleanup();
    return 1;
}
