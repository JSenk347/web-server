/**
 * Summary: Header file defining thread pool interfaces, queue management, and logging prototypes.
 *
 * @file thread_pool.h
 * @authors: Anna Running Rabbit, Joseph Mills, Jordan Senko
 */
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <semaphore.h>
#include <sys/stat.h>

#define NUM_THREADS 4 
#define MAX_SOCKETS 10
#define BUFFER_SIZE 1024

void thread_pool();
void enqueue(int client_socket);
void log_request(int client_fd, char *method, const char *filepath, int status);

#endif