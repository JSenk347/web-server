#ifndef SERVER_H
#define SERVER_H

#include <netdb.h>
#include <stdio.h>

#define PATH_LEN 1024

ssize_t recieve_message(int clientfd, char *buffer);

#endif