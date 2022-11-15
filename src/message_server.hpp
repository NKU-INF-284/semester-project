//
// Created by sarge on 11/14/22.
//

#ifndef SEMESTER_PROJECT_MESSAGE_SERVER_HPP
#define SEMESTER_PROJECT_MESSAGE_SERVER_HPP


#include "server.hpp"

class MessageServer : public Server {
public:
    void on_connection(int) override;

private:
    static bool receive_from_fd(int);
    static void send_message_to_fd(const std::string &message, int fd);
};


#endif //SEMESTER_PROJECT_MESSAGE_SERVER_HPP
