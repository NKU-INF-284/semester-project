#include <iostream>

/**
 * Combined from https://beej.us/guide/bgnet/examples/
 * These are mostly unused, so far
 */
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/**
 * Defines for configuration
 */
#define PORT "3490"      // the port users will be connecting to
#define MAXDATASIZE 256  // max number of bytes we can get at once
#define BACKLOG 10       // how many pending connections queue will hold