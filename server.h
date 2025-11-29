/**
 * Summary: Header file for main.c
 * 
 * @authors: Anna Running Rabbit, Joseph Mills, Jordan Senko
 * @file: main.h
 */
#ifndef SERVER_H
#define SERVER_H

#define NUM_THREADS 4   // arbitrary number can change to w/e
#define MAX_SOCKETS 10  // arbitrary number can change to w/e
#define NUM_CONNECTIONS 5
#define RQ_SIZE 8192

#define METHOD_LEN 10
#define PATH_LEN 1024
#define VERSION_LEN 10

#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include "lib/uthash.h" 



typedef struct HTTPHeader{
    char key[64]; // header name such as "Host" or "Content Type"
    char value[256];
    UT_hash_handle hh; // makes the header hashable
} HTTPHeader; // full header example "Host: www.example.com";

typedef struct HTTPRequest{
    char method[10];
    char path[1024];
    char version[10];
    HTTPHeader *headers; // pointer to head of the HTTPHeader hash table
} HTTPRequest;

int welcome_socket(uint16_t port);
int create_socket(int *socketfd, int domain, int type);
int set_socket_opt(int serverfd);
int bind_socket(int serverfd, uint16_t port, struct sockaddr_in *server_addr,
    socklen_t server_addr_len);
int start_listening(int serverfd);
int handle_client(int serverfd, struct sockaddr_in *server_addr,
    socklen_t server_addr_len);
int accept_client(int serverfd, int *clientfd, struct sockaddr_in *server_addr,
    socklen_t *server_addr_len);
ssize_t recieve_message(int clientfd, char *buffer);
void thread_pool();
void* worker_function(void* arg);
int deq();
//void handle_request();
void handle_request(int clientfd, const char *buffer);
void add_header_to_hash(HTTPHeader **headers, const char *key, const char *value);
void delete_all_headers(HTTPHeader **headers);
//void parse_request(const char *buffer);
void parse_request(const char *buffer, HTTPRequest *rq);


#endif