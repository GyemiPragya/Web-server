#include "server.h"

int main() {
    // Run server on all interfaces, port 8000
    server_start("0.0.0.0", 8000);
    return 0;
}
