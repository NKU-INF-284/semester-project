#include <iostream>

#include "server.hpp"

int main(void) {
    struct addrinfo *servinfo = get_address_info();
    int sockfd = get_socket_file_descriptor(servinfo);

    std::cout << "server: waiting for connections...\n";

    start(sockfd);

    return 0;
}
