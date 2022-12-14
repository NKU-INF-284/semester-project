#include <algorithm>
#include <string>
#include <sstream>
#include <netdb.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
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
        std::string username = "";
        try {
            username = get_username(fd);
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
            if (username.empty()) {
                std::cerr << "client '" << fd << "' terminated and is being cleaned up\n";
            } else {
                std::cerr << "client '" << username << "' (" << fd << ") terminated and is being cleaned up\n";
                connections_mutex.lock();
                connections.erase(username);
                connections_mutex.unlock();
            }

            close(fd);
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
    try {
        std::string line = get_line(fd, MAXDATASIZE);
        connections_mutex.lock();
        send_message_to_all(line, fd, username);
        printf("server: received '%s'\n", line.c_str());
        connections_mutex.unlock();
        return true;
    } catch (connection_terminated &c) {
        return false;
    }
}


std::string replace_all(const std::string &s, const std::string &from, const std::string &to) {
    std::string str(s);
    if (from.empty())
        return "";
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
    return str;
}

void MessageServer::send_message_to_fd(const std::string &message, int fd) {
    std::string formatted_message = replace_all(message, "\n", "\r\n");
    send_buffer(fd, formatted_message.data(), formatted_message.size());
}

/**
 * @return true when connection should continue, false otherwise
 */
bool MessageServer::send_buffer(int fd, const char *buf, size_t bytes_to_send) {
    auto send_res = send(fd, buf, bytes_to_send, 0);
    if (send_res == -1) {
        std::cerr << "got -1 sending to fd: " << fd << std::endl;
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

std::string MessageServer::get_username(int fd) {
    const char *message = "Welcome to Group 4's amazing server!\n"
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

    while (true) {
        start:
        std::string name = get_line(fd, BUFF_SIZE + 1);
        if (name.size() > USERNAME_LEN) {
            send_message_to_fd(warning_message, fd);
            continue;
        }

        name.erase(std::remove_if(name.begin(),
                                  name.end(),
                                  is_invalid_username),
                   name.end());

        if (name.empty()) {
            send_message_to_fd(warning_message, fd);
            continue;
        }

        if (name == "server") {
            send_message_to_fd(name_in_use_message, fd);
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
    if (message.empty() || std::all_of(message.begin(), message.end(), isspace))
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
    const std::string line = std::string(10, '*');
    std::ostringstream ss("\n");
    ss << line;
    ss << "\nWelcome " + username + "!\n";

    if (num_users == 0) {
        ss << "There are no other users online right now.\n";
    } else {
        if (num_users == 1)
            ss << "There is 1 other user online right now:\n";
        else
            ss << "There are " + std::to_string(num_users) + " other users online right now:\n";

        for (auto [name, _]: connections) {
            ss << "\t - " + name + "\n";
        }
    }

    ss << "To send a message, type it out, then press enter.\n"
          "Have fun!\n";
    ss << line;
    ss << "\n";
    send_message_to_fd(ss.str(), fd);
    connections_mutex.unlock();
}

/**
 * gets a line from the client
 * @param fd
 * @param buff_size
 * @return a string ending in a new line;
 */
std::string MessageServer::get_line(int fd, const int buff_size) {
    std::string line;
    char buf[buff_size];

    do {
        auto bytes_to_send = recv(fd, buf, buff_size - 1, 0);

        if (bytes_to_send == -1) {
            std::cerr << "received -1 from fd: " << fd << std::endl;
            perror("recv");
            throw connection_terminated();
        } else if (bytes_to_send == 0) {
            throw connection_terminated();
        } else {
            buf[bytes_to_send] = '\0';  // null terminate the line

            // handle backspace / delete
            if (bytes_to_send == 1 && buf[0] == 127 /* DELETE */) {
                if (!line.empty()) {
                    line.pop_back();
                    send_message_to_fd("\b\b\b   \b\b\b", fd);
                }
                continue;
            } else if (bytes_to_send == 1 && buf[0] == '\b' /*backspace*/) {
                if (!line.empty()) {
                    line.pop_back();
                    send_message_to_fd(" \b", fd);
                }
                continue;
            } else {
                line.append(std::string(buf));
            }
        }
    } while (!line.empty() && line.back() != '\n');

    return line;
}
