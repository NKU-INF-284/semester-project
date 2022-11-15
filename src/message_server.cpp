#include <arpa/inet.h>
#include <cerrno>
#include <netdb.h>
#include <netinet/in.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <thread>

#include "message_server.hpp"

#define MAXDATASIZE 256  // max number of bytes we can get at once

void MessageServer::on_connection(int fd) {
    std::cout << "Connected!!!\n";

    send_message_to_fd("Welcome to Zack's Server!", fd);

    std::thread t([fd]() {
        while (receive_from_fd(fd));  // receive packets from client

        close(fd);
    });
    t.detach();
    // `t` will either die when the connection is closed,
    // or when the server is killed.
}

/**
 * returns true when there is more data to receive
 */
bool MessageServer::receive_from_fd(int fd) {
    char buf[MAXDATASIZE];
    auto bytes_to_send = recv(fd, buf, MAXDATASIZE - 1, 0);

    if (bytes_to_send == -1) {
        perror("recv");
        exit(1);
    } else if (bytes_to_send == 0) {
        return false;  // client has closed the connection
    } else {
        buf[bytes_to_send] = '\0';  // null terminate the buffer

        auto send_res = send(fd, buf, bytes_to_send, 0);
        if (send_res == -1) {
            perror("send");
        } else if (send_res < bytes_to_send) {
            fprintf(stderr, "could not send all bytes of message. sent %ld/%ld\n",
                    send_res, bytes_to_send);
            fprintf(stderr, "TODO: Handle unfinished send\n");
            return false;
        }

        printf("server: received '%s'\n", buf);
        return true;
    }
}

void MessageServer::send_message_to_fd(const std::string &message, int fd) {
    auto bytes_to_send = message.size();
    auto send_res = send(fd, message.data(), bytes_to_send, 0);
    if (send_res == -1) {
        perror("send");
    } else if (send_res < bytes_to_send) {
        fprintf(stderr, "could not send all bytes of message. sent %ld/%ld\n",
                send_res, bytes_to_send);
        fprintf(stderr, "TODO: Handle unfinished send\n");
    }
}

