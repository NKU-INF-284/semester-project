#ifndef SERVER_H
#define SERVER_H

#include <iostream>

void sigchld_handler(int);

class Server {
public:
    Server();

    virtual void start();

    virtual void on_connection(int);

protected:
    int sockfd;

    static void *get_in_addr(struct sockaddr *);

    static struct addrinfo *get_address_info();

    static int get_socket_file_descriptor();


    int accept_connection() const;
};

#endif