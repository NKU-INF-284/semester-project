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

    connections_mutex.lock();
    connections.insert(fd);
    connections_mutex.unlock();
    send_message_to_fd("Welcome to Zack's Server!\n", fd);

    std::thread t([this, fd]() {
        while (handle_connection(fd));  // receive packets from client

        connections_mutex.lock();
        std::cout << "client '" << fd << "' has closed the connection.\n";
        connections.erase(fd);
        connections_mutex.unlock();
        close(fd);
    });
    t.detach();
    // `t` will either die when the connection is closed,
    // or when the server is killed.
}

/**
 * returns true when there is more data to receive
 */
bool MessageServer::handle_connection(int fd) {
    char buf[MAXDATASIZE];
    auto bytes_to_send = recv(fd, buf, MAXDATASIZE - 1, 0);

    if (bytes_to_send == -1) {
        perror("recv");
        exit(1);
    } else if (bytes_to_send == 0) {
        return false;  // client has closed the connection
    } else {
        buf[bytes_to_send] = '\0';  // null terminate the buffer

        connections_mutex.lock();
        bool res;
        for (int conn: connections) {
            res = send_buffer(conn, buf, bytes_to_send);
            std::cout << "sent to '" << conn << "'\n";
        }
        printf("server: received '%s'\n", buf);
        connections_mutex.unlock();
        return res;
    }
}

void MessageServer::send_message_to_fd(const std::string &message, int fd) {
    send_buffer(fd, message.data(), message.size());
}

/**
 * @return true when connection should continue, false otherwise
 */
bool MessageServer::send_buffer(int fd, const char *buf, size_t bytes_to_send) {
    auto send_res = send(fd, buf, bytes_to_send, 0);
    if (send_res == -1) {
        perror("send");
        return false;
    } else if (send_res < bytes_to_send) {
        fprintf(stderr, "could not send all bytes of message. sent %ld/%ld\n",
                send_res, bytes_to_send);
        fprintf(stderr, "TODO: Handle unfinished send\n");
        return false;
    }
    return true;
}

