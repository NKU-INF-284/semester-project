#include <iostream>

#include "server.hpp"

int main(void) {
    Server server;

    std::cout << "server: waiting for connections...\n";

    server.start();

    return 0;
}
