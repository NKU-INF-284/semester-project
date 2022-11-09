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
    static void *get_in_addr(struct sockaddr *);
    static struct addrinfo *get_address_info();
    static int get_socket_file_descriptor();
    static bool receive_from_fd(int);
    int accept_connection() const;
};

#endif