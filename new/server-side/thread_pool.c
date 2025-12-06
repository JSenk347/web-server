#include "thread_pool.h"
#include "server.h"

// --- THREADING GLOBALS ---
pthread_t thread_pool_ids[NUM_THREADS];
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;  // The Lock
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;    // Logging Lock
pthread_cond_t queue_cond_var = PTHREAD_COND_INITIALIZER; // The "Wake Up" Signal

void thread_pool();
void *worker_function(void *arg);

//  Queue of client sockets, waiting for their requests to be serviced
int socket_queue[MAX_SOCKETS]; // Queue of client file descriptors
int queue_head = 0;
int queue_tail = 0;
int queue_count = 0;

/**
 * @brief Initializes the thread pool by creating worker threads.
 */
void thread_pool()
{
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_create(&thread_pool_ids[i], NULL, worker_function, NULL);
        // printf("made a thread\n", NUM_THREADS);
        // NUM_THREAD isn't doing anything without %d ? 
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
        // Get a client from the queue and sleep if empty
        int clientfd = dequeue();
        char buffer[BUFFER_SIZE] = {0};

        // We use recieve_message which calls handle_request
        recieve_message(clientfd, buffer);

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
    pthread_mutex_lock(&queue_mutex); // Thou shalt not unlock

    //----CRITICAL SECTION: START----------------------------------------------
    // Check if queue is full
    if (queue_count < MAX_SOCKETS)
    {
        socket_queue[queue_tail] = client_socket;    // Put ticket in buffer
        queue_tail = (queue_tail + 1) % MAX_SOCKETS; // Move tail
        queue_count++;

        // EYE SPY WITH MY LITTLE eye
        printf("[Producer] Added Client %d to queue. (Queue Size: %d)\n", client_socket, queue_count);
        // Wake up sleeping beauty (thread)
        pthread_cond_signal(&queue_cond_var);
    }
    else
    {
        // Queue full
        printf("Queue full! Dropping connection.\n");
        close(client_socket);
    }
    //----CRITICAL SECTION: END------------------------------------------------

    pthread_mutex_unlock(&queue_mutex); // Unlock the door
}

/**
 * @brief Dequeues a client socket from the socket queue for processing by worker threads.
 *
 * @return The dequeued client socket file descriptor.
 */
int dequeue()
{
    pthread_mutex_lock(&queue_mutex); // Thou shalt not unlock

    //----CRITICAL SECTION: START----------------------------------------------
    // Wait while queue is empty (Condition Variable)
    while (queue_count == 0)
    {
        // Sleep until signaled. Releases lock automatically while sleeping.
        pthread_cond_wait(&queue_cond_var, &queue_mutex);
    }

    // Woke up and have the lock! Take the item.
    int client_socket = socket_queue[queue_head];
    queue_head = (queue_head + 1) % MAX_SOCKETS;
    queue_count--;

    // EYE SPY WITH MY LITTLE eye
    printf("  [Worker %lu] Dequeued Client %d. Queue Size Remaining: %d\n", pthread_self(), client_socket, queue_count);
    //----CRITICAL SECTION: END------------------------------------------------

    pthread_mutex_unlock(&queue_mutex); // Unlock the door
    return client_socket;
}

/**
 * @brief //TODO
 *
 * @param client_fd
 * @param method
 * @param path
 * @param status
 *
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