/**
 * Summary: Header file for client-side response parsing and file saving utilities.
 *
 * @file c_http_parser.h
 * @authors: Anna Running Rabbit, Joseph Mills, Jordan Senko
 */
#ifndef C_HTTP_PARSER_H
#define C_HTTP_PARSER_H

#define FILE_NAME_LEN 256
#define BUFFER_SIZE 1024

#include <stdio.h>

void send_request(int sockfd, const char request[]);
void read_remaining_body_bytes(long remaining_bytes, long *total_written, char *buffer, int sockfd, FILE *outfile, char *file_name);

#endif