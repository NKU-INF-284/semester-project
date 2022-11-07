#ifndef SERVER_H
#define SERVER_H

#include <iostream>

/**
 * Function declarations
 */
void sigchld_handler(int);
void *get_in_addr(struct sockaddr *);
struct addrinfo *get_address_info();
int get_socket_file_descriptor(struct addrinfo *);
void start(int);

#endif