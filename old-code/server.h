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
#define PATH_LEN 1024

#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "common.h"
#include <sys/stat.h>
#include "lib/uthash.h" 

#endif