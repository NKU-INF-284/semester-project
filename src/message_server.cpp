#include <algorithm>
#include <arpa/inet.h>
#include <string>
#include <sstream>
#include <string_view>
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
#define USERNAME_LEN 10
#define VAL(x) #x
#define TOSTRING(str) VAL(str)


void MessageServer::on_connection(int fd) {
    std::cout << "Connected!!!\n";

    std::thread t([this, fd]() {
        try {
            const std::string username = get_username(fd);
            std::cout << "'" << username << "' joined the chat." << std::endl;

            welcome_user(fd, username);

            connections_mutex.lock();
            connections[username] = fd;
            connections_mutex.unlock();

            send_message_to_all("\"" + username + "\" has joined the chat.", fd, "server");

            // after username is obtained, handle messages
            while (handle_connection(fd, username));  // receive packets from client

            connections_mutex.lock();
            std::cout << "client '" << fd << "' has closed the connection.\n";
            connections.erase(username);
            connections_mutex.unlock();
            close(fd);
            send_message_to_all("\"" + username + "\" has left the chat.", -1, "server");
        } catch (connection_terminated &e) {
            std::cerr << "Error getting username. Connection terminated." << std::endl;
            return;
        }
    });
    t.detach();
    // `t` will either die when the connection is closed,
    // or when the server is killed.
}

/**
 * returns true when there is more data to receive
 */
bool MessageServer::handle_connection(int fd, const std::string &username) {
    char buf[MAXDATASIZE];
    auto bytes_to_send = recv(fd, buf, MAXDATASIZE - 1, 0);

    if (bytes_to_send == -1) {
        perror("recv");
        exit(1);
    } else if (bytes_to_send == 0) {
        return false;  // client has closed the connection
    } else {
        // TODO: Filter buffer
        buf[bytes_to_send] = '\0';  // null terminate the buffer

        bool res = true;
        connections_mutex.lock();
        send_message_to_all(std::string(buf), fd, username);
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

bool is_invalid(char c) {
    return !std::isprint(c);
}

bool is_invalid_username(char c) {
    bool is_alpha = (c >= 97 && c <= 122) || (c >= 65 && c <= 90);
    bool is_numeric = c >= 48 && c <= 57;
    bool is_alphanumeric = is_alpha || is_numeric;
    return std::isspace(c) || !is_alphanumeric;
}

bool is_invalid_message(char c) {
    if (std::isspace(c))
        return false;
    else return is_invalid(c);
}

bool buff_contains(const char *buf, long len, char c) {
    for (long i = 0; i < len; i++)
        if (buf[i] == c)
            return true;
    return false;
}

std::string MessageServer::get_username(int fd) {
    // TODO: make sure two people can't have the same username
    // TODO: wait for newline
    const char *message = "Welcome to Zack Sargent's Server!\n"
                          "Please enter your username: ";
    const char *warning_message = "Please enter an alphanumeric username less than "
                                  TOSTRING(USERNAME_LEN)
                                  " characters!\n"
                                  "Please enter your username: ";
    const char *name_in_use_message = "That username is currently in use! Please try again.\n"
                                      "Please enter your username: ";

    send_message_to_fd(message, fd);
    const int null_terminator = 1;
    const int new_line = 1;
    const int BUFF_SIZE = USERNAME_LEN + null_terminator + new_line;
    char username[BUFF_SIZE];
    bool shouldProcess = true;
    ssize_t bytes_received;

    while (true) {
        start:
        bytes_received = recv(fd, username, BUFF_SIZE, 0);
        std::cout << "bytes: " << bytes_received << std::endl;

        if (bytes_received > USERNAME_LEN || !shouldProcess) {
            shouldProcess = buff_contains(username, bytes_received, '\n');
            if (shouldProcess)
                send_message_to_fd(warning_message, fd);
            continue;
        }

        if (bytes_received == -1) {
            perror("recv");
            throw connection_terminated();
        } else if (bytes_received == 0) {
            throw connection_terminated();
        }

        username[bytes_received] = '\0'; // null terminate for string conversion

        std::string name(username);

        name.erase(std::remove_if(name.begin(),
                                  name.end(),
                                  is_invalid_username),
                   name.end());

        if (name.empty()) {
            send_message_to_fd(warning_message, fd);
            continue;
        }

        connections_mutex.lock();
        for (auto [existing_username, _]: connections) {
            if (name == existing_username) {
                send_message_to_fd(name_in_use_message, fd);
                connections_mutex.unlock();
                goto start; // goto is needed to break from nested loop.
            }
        }
        connections_mutex.unlock();

        return name;
    }
}

void MessageServer::send_message_to_all(std::string message, int origin, const std::string &username) {
    message.erase(std::remove_if(message.begin(),
                                 message.end(),
                                 is_invalid_message),
                  message.end());
    if (message.empty() || message == "\n")
        return;

    // build message
    std::stringstream msg("");
    msg << "<";
    msg << username;
    msg << "> ";
    msg << message;
    // append newline if needed
    if (message.back() != '\n')
        msg << '\n';

    for (auto [_, conn]: connections) {
        if (conn != origin) {
            send_message_to_fd(msg.str(), conn);
            std::cout << "sent to '" << conn << "' (" << username << ")\n";
        }
    }
}

void MessageServer::welcome_user(int fd, const std::string &username) {
    connections_mutex.lock();
    auto num_users = connections.size();
    std::string line = std::string(10, '*');
    std::stringstream ss("\n");
    ss << line;
    ss << "\nWelcome " + username + "!\n";

    if (num_users == 0) {
        ss << "There are no other users online right now.\n";
    } else {
        if (num_users == 1)
            ss << "There is " + std::to_string(num_users) + " other user online right now:\n";
        else
            ss << "There are " + std::to_string(num_users) + " other users online right now:\n";

        for (auto [name, _]: connections) {
            ss << "\t -" + name + "\n";
        }
    }

    ss << "To send a message, type it out, then press enter.\n"
          "Have fun!\n";
    ss << line;
    ss << "\n\n\n";
    send_message_to_fd(ss.str(), fd);
    connections_mutex.unlock();
}

