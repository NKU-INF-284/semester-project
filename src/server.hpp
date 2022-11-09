#ifndef SERVER_H
#define SERVER_H

#include <iostream>

void sigchld_handler(int);

class Server {
   public:
    Server();
    void start();

   private:
    int sockfd;
    void *get_in_addr(struct sockaddr *);
    struct addrinfo *get_address_info();
    int get_socket_file_descriptor();
    static bool receive_from_fd(int);
    int accept_connection();
};

#endif