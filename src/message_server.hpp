//
// Created by sarge on 11/14/22.
//

#ifndef SEMESTER_PROJECT_MESSAGE_SERVER_HPP
#define SEMESTER_PROJECT_MESSAGE_SERVER_HPP


#include <unordered_map>
#include <mutex>
#include "server.hpp"

class MessageServer : public Server {
public:
    void on_connection(int) override;

private:
    bool handle_connection(int fd, const std::string &username);

    void send_message_to_all(std::string message, int origin, const std::string &username);

    static void send_message_to_fd(const std::string &message, int fd);

    static bool send_buffer(int fd, const char *buf, size_t bytes_to_send);

    std::string get_username(int fd);

    void welcome_user(int fd, const std::string &username);

    static std::string get_line(int fd, int buff_size);

    std::unordered_map<std::string, int> connections;
    // could also be: std::unordered_set<std::pair<std::string, int>>> connections;
    std::mutex connections_mutex;
};

class connection_terminated : public std::exception {

};

#endif //SEMESTER_PROJECT_MESSAGE_SERVER_HPP
