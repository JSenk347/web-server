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
#define BUFFER_SIZE 1024

#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>

int welcome_socket(uint16_t port);
int create_wel_socket(int *serverfd);
int set_socket_opt(int serverfd);
int bind_socket(int serverfd, uint16_t port, struct sockaddr_in *server_addr,
    socklen_t server_addr_len);
void thread_pool();
void* worker_function(void* arg);
int deq();
void handle_request();


#endif