#ifndef SERVER_H
#define SERVER_H

#include <iostream>

void sigchld_handler(int);

class Server {
   public:
    Server();
    ~Server();
    void start();

   private:
    struct addrinfo *servinfo;
    int sockfd;
    void *get_in_addr(struct sockaddr *);
    struct addrinfo *get_address_info();
    int get_socket_file_descriptor();
};

#endif