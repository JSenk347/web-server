/**
 * Summary: Simple TCP server 'main' function to set up sockets
 *
 * @authors: Anna Running Rabbit, Joseph Mills, Jordan Senko
 *
 * @file main.c
 */

#include "main.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <bits/pthreadtypes.h>
#include <semaphore.h>

sem_t sem_items;    // Counts number of items in the queue
sem_t sem_q;        // Semaphore used for locking/unlocking critical sections
struct sockaddr_in socket_q[MAX_SOCKETS]; // Queue of sockets provided by the main thread. Sockets consumed by the worker threads

// Common syscalls:
// socket(), bind(), connect(), recv(), send(), accept()

int main()
{
    uint16_t port = 6767;                      // Port number for the server to listen on
    int welcome_sockfd = welcome_socket(port); // Create welcome socket to listen for incoming connections

    thread_pool(); // Initialize thread pool to handle incoming connections

    return 0;
}

/**
* @brief Initializes the servers welcome socket to listen for incoming TCP
*        connections on the specified port.
*
* @param port The port number on which the server will listen for incoming connections.
* @return the welcome socket file descriptor on success, or -1 on failure.
*/
int init_welcome_socket(uint16_t port)
{
    int sockfd;
    struct sockaddr_in server_addr;

    // Create the socket (IPv4, TCP)
    sockfd = socket(AF_INET, SOCK_STREAM, 0); // AF_INET = Use IPv4; SOCK_STREAM = specifies stream socket type who's default protocol is TCP

    /* error checking if socket creation failed */
    if (sockfd < 0)
    {
        perror("socket");
        return -1;
    }

    // Set address/port reusable (optional, for quick restarts)
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr)); // wipes any garbo from the server_addr structure, filling &server_addr with 0 for sizeof(server_addr) bytes
    server_addr.sin_family = AF_INET;             // Specifies the server address TYPE to IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY;     // Listen on all interfaces -> any of the machines IP addresses
    server_addr.sin_port = htons(port);           // Set port number

    // Bind the socket to the address and port -> reserves this port and IP addr for this socket
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind");
        close(sockfd);
        return -1;
    }

    // Listen for incoming connections (max 5 in the queue)
    if (listen(sockfd, 5) < 0)
    {
        perror("listen");
        close(sockfd);
        return -1;
    }

    return sockfd; // This is your "welcome socket"
}

/**
 * @brief Creates a pool of worker threads and assigns them to the worker_function
 */
void thread_pool()
{
    pthread_t threadPool[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_create(&threadPool[i], NULL, worker_function, NULL);
    }
}

/**
* @brief Function executed by each worker thread to handle incoming client requests.
*
* @param arg Pointer to any arguments passed to the thread (not used here).
*/
void* worker_function(void* arg) {
    while (1) {
        sem_wait(&sem_items);        // Puts process to sleep until there's at least one item. Will be posted by a producer?
        sem_wait(&sem_q);            // Puts process to sleep if the critical section is being accessed by another process

        int clientSocket = deq();

        sem_post(&sem_q);            // Unlock access to the q

        handleRequest(clientSocket);
        close(clientSocket);
    }
}