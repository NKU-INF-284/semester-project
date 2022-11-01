#include <iostream>

#include "server.hpp"

/**
 *Taken from Beej's Guide to Network Programming
 * https://beej.us/guide/bgnet/examples/server.c
 */
void sigchld_handler(int s) {
    (void)s;  // quiet unused variable warning

    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;

    errno = saved_errno;
}

/**
 *Taken from Beej's Guide to Network Programming
 * https://beej.us/guide/bgnet/examples/server.c
 */
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(void) {
    /**
     * Get address info:
     * (Note this is still mostly taken from Beej
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

    /**
     * Bind the socket to a file descriptor
     * (Note this is still mostly taken from Beej
     * (https://beej.us/guide/bgnet/examples/))
     */
    int sockfd;
    struct addrinfo *p;
    struct sigaction sa;

    for (p = servinfo; p != NULL; p = p->ai_next) {
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

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler;  // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    /**
     * Forever, accept connections
     * Taken from https://beej.us/guide/bgnet/examples/server.c
     */
    while (true) {                           // main accept() loop
        struct sockaddr_storage their_addr;  // connector's address information
        socklen_t sin_size = sizeof their_addr;
        int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        char s[INET6_ADDRSTRLEN];
        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);

        printf("server: got connection from %s\n", s);

        if (!fork()) {      // this is the child process
            close(sockfd);  // child doesn't need the listener

            while (true) {  // recieve packets from client until the
                            // connection is closed
                char buf[MAXDATASIZE];
                int numbytes = recv(new_fd, buf, MAXDATASIZE - 1, 0);

                if (numbytes == -1) {
                    perror("recv");
                    exit(1);
                } else if (numbytes == 0) {
                    break;  // client has closed the connection
                }

                buf[numbytes] = '\0';  // null terminate the buffer

                int bytes_to_send = numbytes;
                int send_res;

                // This was written by me
                send_res = send(new_fd, buf, bytes_to_send, 0);
                if (send_res == -1) {
                    perror("send");
                } else if (send_res < bytes_to_send) {
                    fprintf(stderr,
                            "could not send all bytes of message. sent %d/%d\n",
                            send_res, bytes_to_send);
                    fprintf(stderr, "TODO: Handle unfinished send\n");
                }

                printf("server: received '%s'\n", buf);
            }

            close(new_fd);
            exit(0);
        } else {
            close(new_fd);
        }
    }

    return 0;
}
