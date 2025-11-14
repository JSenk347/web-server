/**
 * Summary: Header file for main.c
 * 
 * @authors: Anna Running Rabbit, Joseph Mills, Jordan Senko
 * @file: main.h
 */

#define NUM_THREADS 4   // arbitrary number can change to w/e
#define MAX_SOCKETS 10  // arbitrary number can change to w/e

int init_welcome_socket(uint16_t port);
void thread_pool();
void* worker_function(void* arg);