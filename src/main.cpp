#include <iostream>

#include "server.hpp"

int main(void) {
    /**
     * Get address info:
     * (Note this is still heavily taken from Beej
     * (https://beej.us/guide/bgnet/examples/))
     */
    struct addrinfo hints;
    struct addrinfo *servinfo;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int rv = getaddrinfo(NULL, PORT, &hints, &servinfo);
    if (rv != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // create file descriptor
    // make call to `getaddrinfo`
    // loop through all results and bind to first avaliable
    // forever, accept connections

    std::cout << "Hello World!\n";
    std::cout << "from INF 284\n";
}
