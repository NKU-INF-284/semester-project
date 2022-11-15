//
// Created by sarge on 11/14/22.
//

#ifndef SEMESTER_PROJECT_MESSAGE_SERVER_HPP
#define SEMESTER_PROJECT_MESSAGE_SERVER_HPP


#include <unordered_set>
#include <mutex>
#include "server.hpp"

class MessageServer : public Server {
public:
    void on_connection(int) override;

private:
    bool handle_connection(int);
    static void send_message_to_fd(const std::string &message, int fd);
    static bool send_buffer(int fd, const char *buf, size_t bytes_to_send);

    std::unordered_set<int> connections;
    std::mutex connections_mutex;
};


#endif //SEMESTER_PROJECT_MESSAGE_SERVER_HPP
