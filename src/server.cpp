#include "server.hpp"

/**
 * Combined from https://beej.us/guide/bgnet/examples/
 * These are mostly unused, so far
 */
#include <arpa/inet.h>
#include <cerrno>
#include <netdb.h>
#include <netinet/in.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

/**
 * Defines for configuration
 */
#define PORT "3490"      // the port users will be connecting to
#define BACKLOG 10       // how many pending connections queue will hold

Server::Server() { this->sockfd = get_socket_file_descriptor(); }

/**
 *Taken from Beej's Guide to Network Programming
 * https://beej.us/guide/bgnet/examples/server.c
 */
void sigchld_handler(int s) {
    (void) s;  // quiet unused variable warning

    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while (waitpid(-1, nullptr, WNOHANG) > 0);

    errno = saved_errno;
}

/**
 *Taken from Beej's Guide to Network Programming
 * https://beej.us/guide/bgnet/examples/server.c
 */
void *Server::get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

struct addrinfo *Server::get_address_info() {
    /**
     * Get address info:
     * (Note this is still mostly taken from Beej
     * (https://beej.us/guide/bgnet/examples/))
     */
    struct addrinfo hints{};
    struct addrinfo *servinfo;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int rv = getaddrinfo(nullptr, PORT, &hints, &servinfo);
    if (rv != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }
    return servinfo;
}

int Server::get_socket_file_descriptor() {
    struct addrinfo *servinfo = get_address_info();

    /**
     * Bind the socket to a file descriptor
     * (Note this is still mostly taken from Beej
     * (https://beej.us/guide/bgnet/examples/))
     */
    int sockfd;
    struct addrinfo *p;

    // traverse linked list starting at servinfo
    for (p = servinfo; p != nullptr; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==
            -1) {
            perror("server: socket");
            continue;
        }

        int yes = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) ==
            -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo);  // all done with this structure

    if (p == nullptr) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    struct sigaction sa{};
    sa.sa_handler = sigchld_handler;  // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, nullptr) == -1) {
        perror("sigaction");
        exit(1);
    }

    return sockfd;
}

void Server::start() {
    /**
     * Forever, accept connections
     * Inspired by https://beej.us/guide/bgnet/examples/server.c
     */
    while (true) {  // main accept() loop
        std::cout << "server: waiting for connections...\n";
        int new_fd = accept_connection();
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        on_connection(new_fd);
    }
}

int Server::accept_connection() const {
    struct sockaddr_storage their_addr{};  // connector's address information
    socklen_t sin_size = sizeof their_addr;
    int new_fd = accept(this->sockfd, (struct sockaddr *) &their_addr, &sin_size);

    char s[INET6_ADDRSTRLEN];
    inet_ntop(their_addr.ss_family,
              get_in_addr((struct sockaddr *) &their_addr), s, sizeof s);

    printf("server: got connection from %s\n", s);
    return new_fd;
}

/**
 * Can be overridden by children
 * @param fd - the file descriptor for the client
 */
void Server::on_connection(int fd) {
    std::cout << "Connected!\n";
    close(fd);
}
