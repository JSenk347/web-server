/**
 * Summary: Implementation of the thread pool, task queue, and worker threads for concurrent request handling.
 *
 * @file thread_pool.c
 * @authors: Anna Running Rabbit, Joseph Mills, Jordan Senko
 */
#include "thread_pool.h"
#include "http_parser.h"
#include "server.h"

#include <unistd.h>

// --- THREADING GLOBALS ---
pthread_t thread_pool_ids[NUM_THREADS];
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;  // queue mutex lock
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;    // logging mutex lock
pthread_cond_t queue_cond_var = PTHREAD_COND_INITIALIZER; // thread "Wake Up" signal

void thread_pool();
void *worker_function(void *arg);
void enqueue(int client_socket);
int dequeue();
void log_request(int client_fd, char *method, const char *filepath, int status);

// queue of client sockets, waiting for their requests to be serviced
int socket_queue[MAX_SOCKETS]; // queue of client file descriptors
int queue_head = 0;
int queue_tail = 0;
int queue_count = 0;


// --- FUNCTIONS ---
/**
 * @brief Initializes the thread pool by creating worker threads.
 */
void thread_pool()
{
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_create(&thread_pool_ids[i], NULL, worker_function, NULL); 
    }
    printf("Thread pool initialized with %d workers.\n", NUM_THREADS);
}

/**
 * @brief Function executed by each worker thread to handle incoming client requests.
 *
 * @param arg Pointer to any arguments passed to the thread (not used here).
 */
void *worker_function(void *arg)
{
    while (1)
    {
        // get a client from the queue and sleep if empty
        int clientfd = dequeue();
        //sleep(1); for testing
        char buffer[BUFFER_SIZE] = {0};

        // we use recieve_message which calls handle_request
        receive_message(clientfd, buffer);

        close(clientfd);
    }
    return 0;
}

/**
 * @brief Enqueues a client socket into the socket queue for processing by worker threads.
 *
 * @param client_socket The client socket file descriptor to be enqueued.
 */
void enqueue(int client_socket)
{
    pthread_mutex_lock(&queue_mutex); // thou shalt not access!

    //----CRITICAL SECTION: START----------------------------------------------
    if (queue_count < MAX_SOCKETS) // check if queue is full
    {
        socket_queue[queue_tail] = client_socket;    // put ticket in buffer
        queue_tail = (queue_tail + 1) % MAX_SOCKETS; // move tail
        queue_count++;

        printf(" - [Producer] Enqueued client %d | Curr Queue Size: %d\n", client_socket, queue_count);
        pthread_cond_signal(&queue_cond_var); // wake up sleeping beauty (thread)
    }
    else
    {
        printf(" - ⚠️ Warning: queue full! Dropping connection.\n");
        close(client_socket);
    }
    //----CRITICAL SECTION: END------------------------------------------------

    pthread_mutex_unlock(&queue_mutex); // thou shall access
}

/**
 * @brief Dequeues a client socket from the socket queue for processing by worker threads.
 *
 * @return The dequeued client socket file descriptor.
 */
int dequeue()
{
    pthread_mutex_lock(&queue_mutex); // thou shall not access

    //----CRITICAL SECTION: START----------------------------------------------
    while (queue_count == 0) // wait while queue is empty
    {
        // sleep until signaled. releases lock when put to sleep.
        pthread_cond_wait(&queue_cond_var, &queue_mutex);
    }

    // thread has woken up and has the lock! take the item from the queue
    int client_socket = socket_queue[queue_head];
    queue_head = (queue_head + 1) % MAX_SOCKETS;
    queue_count--;

    printf("    - [Worker %lu] Dequeued client %d | Curr Queue Size: %d\n", pthread_self(), client_socket, queue_count);
    //----CRITICAL SECTION: END------------------------------------------------

    pthread_mutex_unlock(&queue_mutex); // thou shall access
    return client_socket;
}

/**
 * @brief Thread-safe logging of HTTP requests to the server console.
 *
 * @param client_fd Socket file descriptor of the client being served
 * @param method HTTP method used in the request (e.g., "GET")
 * @param filepath Path of the requested resource
 * @param status HTTP status code returned to the client (e.g., 200, 404, etc.)
 */
void log_request(int client_fd, char *method, const char *filepath, int status)
{
    pthread_mutex_lock(&log_mutex);

    //----CRITICAL SECTION: START----------------------------------------------
    printf("[Worker thread: %lu] %s %s -> Status: %d\n", pthread_self(),
           method, filepath, status);
    //----CRITICAL SECTION: END------------------------------------------------

    pthread_mutex_unlock(&log_mutex);
}