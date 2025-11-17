#define _CRT_SECURE_NO_WARNINGS
#include "http_handler.h"
#include "file_serving.h"
#include "logging.h"
#include "page_replacement.h"  // NEW: Include page replacement
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <winsock2.h>
#include <windows.h>

// Metrics counters (global)
int metric_requests_total = 0;
int metric_active_connections = 0;

// Helper: send HTTP response
static void send_response(SOCKET client_sock, const char *status, const char *content_type, const char *body) {
    char response[8192];  // Increased for larger responses
    int len = snprintf(response, sizeof(response),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n%s",
        status, content_type, strlen(body), body);
    send(client_sock, response, len, 0);
}

// NEW: Parse reference string "7,0,1,2" into int array
int* parse_reference_string(const char* refStr, int *out_len) {
    char *copy = _strdup(refStr);
    int *refs = (int*)malloc(256 * sizeof(int));
    int count = 0;
    char *token = strtok(copy, ", ");
    while (token && count < 256) {
        refs[count++] = atoi(token);
        token = strtok(NULL, ", ");
    }
    free(copy);
    *out_len = count;
    return refs;
}

// NEW: Handle page replacement visualization request
void handle_page_replacement(SOCKET client_sock, const char *query_string) {
    // Parse query string: refs=7,0,1,2&frames=3&algo=fifo
    char refs_str[512] = {0};
    int num_frames = 3;
    char algo[32] = "fifo";
    
    // Simple query parsing (you can improve this)
    const char *p = strstr(query_string, "refs=");
    if (p) {
        sscanf(p, "refs=%511[^&]", refs_str);
    }
    p = strstr(query_string, "frames=");
    if (p) {
        sscanf(p, "frames=%d", &num_frames);
    }
    p = strstr(query_string, "algo=");
    if (p) {
        sscanf(p, "algo=%31[^&]", algo);
    }

    // Parse reference string
    int num_refs;
    int *refs = parse_reference_string(refs_str, &num_refs);
    
    // Run algorithm
    FrameState *steps = NULL;
    int step_count = 0;
    
    if (strcmp(algo, "fifo") == 0) {
        steps = simulate_fifo(refs, num_refs, num_frames, &step_count);
    } else if (strcmp(algo, "lru") == 0) {
        steps = simulate_lru(refs, num_refs, num_frames, &step_count);
    } else if (strcmp(algo, "optimal") == 0) {
        steps = simulate_optimal(refs, num_refs, num_frames, &step_count);
    }
    
    // Build JSON response
    char json[8192];
    int pos = snprintf(json, sizeof(json), "{\"steps\":[");
    
    for (int i = 0; i < step_count && pos < sizeof(json) - 100; i++) {
        pos += snprintf(json + pos, sizeof(json) - pos, 
            "{\"ref\":%d,\"fault\":%d,\"replaced\":%d,\"frames\":[",
            steps[i].ref_page, steps[i].fault, steps[i].replaced);
        
        for (int j = 0; j < steps[i].num_frames; j++) {
            pos += snprintf(json + pos, sizeof(json) - pos, 
                "%d%s", steps[i].frames[j], j < steps[i].num_frames - 1 ? "," : "");
        }
        pos += snprintf(json + pos, sizeof(json) - pos, "]}%s", i < step_count - 1 ? "," : "");
    }
    pos += snprintf(json + pos, sizeof(json) - pos, "]}");
    
    send_response(client_sock, "200 OK", "application/json", json);
    
    // Cleanup
    free_frame_states(steps, step_count);
    free(refs);
}

// Process a single client request
void process_job(SOCKET client_sock, const char *client_ip) {
    char buffer[4096];
    int bytes_received;

    metric_active_connections++;
    metric_requests_total++;

    bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        closesocket(client_sock);
        metric_active_connections--;
        return;
    }

    buffer[bytes_received] = '\0';
    log_info("Request from %s:\n%s", client_ip, buffer);

    // Parse request line
    char method[16], path[512], version[16];
    sscanf(buffer, "%s %s %s", method, path, version);

    // Routing
    if (strcmp(method, "GET") == 0) {
        if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
            // Serve your main HTML page
            serve_static_file(client_sock, "./www", "/index.html");
        } 
        else if (strncmp(path, "/visualize?", 11) == 0) {
            // NEW: Handle page replacement visualization
            handle_page_replacement(client_sock, path + 11);
        }
        else if (strcmp(path, "/metrics") == 0) {
            char metrics[256];
            snprintf(metrics, sizeof(metrics),
                "requests_total %d\nactive_connections %d\n",
                metric_requests_total, metric_active_connections);
            send_response(client_sock, "200 OK", "text/plain", metrics);
        } 
        else {
            send_response(client_sock, "404 Not Found", "text/plain", "Not Found");
        }
    } else {
        send_response(client_sock, "501 Not Implemented", "text/plain", "Method Not Implemented");
    }

    closesocket(client_sock);
    metric_active_connections--;
}
