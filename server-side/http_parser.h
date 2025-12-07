#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include "server.h"

#define BUFFER_SIZE 1024

void send_error_response(const char *filepath, int clientfd, int status_code);
const char *get_mime_type(const char *filepath);
void handle_request(int clientfd, const char *buffer);
void serve_file(int clientfd, const char *filepath, off_t filesize);
ssize_t receive_message(int clientfd, char *buffer);

#endif