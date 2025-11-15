#ifndef COMMON_H
#define COMMON_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdint.h> //contains uint16_t struct
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#define PORT 6767

int socket_to_string(int sockfd, struct sockaddr_in server_addr);
int client_socket(uint16_t port, struct sockaddr_in server_addr);
int connect_client(int clientfd, struct sockaddr_in server_addr);

#endif