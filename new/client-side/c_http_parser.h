#ifndef C_HTTP_PARSER_H
#define C_HTTP_PARSER_H

#include <netinet/in.h>
#include <stdio.h>

#define FILE_NAME_LEN 32
#define BUFFER_SIZE 1024

void send_request(int sockfd, const char request[]);

#endif