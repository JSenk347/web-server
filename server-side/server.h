/**
 * Summary: Header file defining core server constants, socket prototypes, and buffer sizes.
 *
 * @file server.h
 * @authors: Anna Running Rabbit, Joseph Mills, Jordan Senko
 */
#ifndef SERVER_H
#define SERVER_H

#include <netdb.h>
#include <stdio.h>

#define PATH_LEN 2048

ssize_t recieve_message(int clientfd, char *buffer);

#endif