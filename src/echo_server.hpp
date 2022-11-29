#ifndef SEMESTER_PROJECT_ECHO_SERVER_HPP
#define SEMESTER_PROJECT_ECHO_SERVER_HPP

#include "server.hpp"

class EchoServer : public Server {
public:
    void on_connection(int) override;
private:
    static bool receive_from_fd(int);
};


#endif //SEMESTER_PROJECT_ECHO_SERVER_HPP
