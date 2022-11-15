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

#include "echo_server.hpp"

#define MAXDATASIZE 256  // max number of bytes we can get at once

void EchoServer::on_connection(int fd) {
    std::cout << "Connected!\n";
    if (!fork()) {  // spawn a child process to not block the main loop
        close(this->sockfd);  // child doesn't need the listener

        while (receive_from_fd(fd));  // receive packets from client

        close(fd);
        exit(0);
    } else {
        close(fd);
    }
}

/**
 * returns true when there is more data to receive
 */
bool EchoServer::receive_from_fd(int new_fd) {
    char buf[MAXDATASIZE];
    auto bytes_to_send = recv(new_fd, buf, MAXDATASIZE - 1, 0);

    if (bytes_to_send == -1) {
        perror("recv");
        exit(1);
    } else if (bytes_to_send == 0) {
        return false;  // client has closed the connection
    } else {
        buf[bytes_to_send] = '\0';  // null terminate the buffer

        auto send_res = send(new_fd, buf, bytes_to_send, 0);
        if (send_res == -1) {
            perror("send");
        } else if (send_res < bytes_to_send) {
            fprintf(stderr, "could not send all bytes of message. sent %ld/%ld\n",
                    send_res, bytes_to_send);
            fprintf(stderr, "TODO: Handle unfinished send\n");
        }

        printf("server: received '%s'\n", buf);
        return true;
    }
}
