/**
 * Summary: Simple TCP server 'main' function to set up sockets
 *
 * @file server.c
 * @authors: Anna Running Rabbit, Joseph Mills, Jordan Senko
 */

#include "server.h"
#include "common.h"

sem_t sem_items;                          // Counts number of items in the queue
sem_t sem_q;                              // Semaphore used for locking/unlocking critical sections
struct sockaddr_in socket_q[MAX_SOCKETS]; // Queue of sockets provided by the main thread. Sockets consumed by the worker threads

// Common syscalls:
// socket(), bind(), connect(), recv(), send(), accept()

int main()
{
    int welcome_sockfd = welcome_socket(PORT); // Create welcome socket to listen for incoming connections

    // thread_pool(); // Initialize thread pool to handle incoming connections

    return 0;
}

/**
 * @brief Initializes the servers welcome socket to listen for incoming TCP
 *        connections on the specified port.
 *
 * @param port The port number on which the server will listen for incoming connections.
 * @return the welcome socket file descriptor on success, or -1 on failure.
 */
int welcome_socket(uint16_t port)
{
    int serverfd;
    struct sockaddr_in server_addr;
    socklen_t server_addr_len = sizeof(server_addr);

    // AF_INET = Use IPv4
    // SOCK_STREAM = specifies stream socket type who's default protocol is TCP
    if (create_socket(&serverfd, AF_INET, SOCK_STREAM) < 0) return -1;

    if (set_socket_opt(serverfd) < 0)
    {
        close(serverfd);
        return -1;
    }

    if (bind_socket(serverfd, port, &server_addr, server_addr_len) < 0)
    {
        close(serverfd);
        return -1;
    }

    if (start_listening(serverfd) < 0)
    {
        close(serverfd);
        return -1;
    }

    if (handle_client(serverfd, &server_addr, server_addr_len) < 0)
    {
        close(serverfd);
        return -1;
    }

    return serverfd; // Returns the file descriptor of the socket
}

/**
 * @brief Creates a socket with the specified domain and type.
 * 
 * @param socketfd Pointer to an integer where the created socket file descriptor will be stored.
 * @param domain The communication domain (e.g., AF_INET for IPv4).
 * @param type The communication type (e.g., SOCK_STREAM for TCP).
 * @return 0 on success, -1 on failure.
 */
int create_socket(int *socketfd, int domain, int type)
{
    // Validate input pointer
    if (socketfd == NULL)
    {
        fprintf(stderr, "Invalid pointer passed to create_wel_socket\n");
        return -1;
    }

    // Create the socket
    *socketfd = socket(/*AF_INET*/domain, /*SOCK_STREAM*/type, 0);

    // Error check socket creation failure
    if (*socketfd < 0)
    {
        perror("\nwelcome socket creation failed");
        return -1;
    }

    return 0;
}

/**
 * @brief Sets socket options for the given server socket file descriptor.
 *
 * @param serverfd The server socket file descriptor.
 * @return 0 on success, -1 on failure.
 */
int set_socket_opt(int serverfd)
{
    // Set address/port reusable (optional, for quick restarts)
    int opt = 1;
    
    if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        // Error check setsockopt failure
        perror("\nsetsockopt failed");
        return -1;
    }
    return 0;
}

/**
 * @brief Binds the server socket to the specified port and address.
 * 
 * @param serverfd The server socket file descriptor.
 * @param port The port number to bind the socket to.
 * @param server_addr Pointer to the sockaddr_in structure to hold the server address.
 * @param server_addr_len The length of the server_addr structure.
 * @return 0 on success, -1 on failure.
 */
int bind_socket(int serverfd, uint16_t port, struct sockaddr_in *server_addr,
    socklen_t server_addr_len)
{
    /* wipes any garbo from the server_addr structure, filling &server_addr
       with 0 for sizeof(server_addr) bytes */
    memset(server_addr, 0, server_addr_len);

    server_addr->sin_family = AF_INET;             // Specifies the server address TYPE to IPv4
    server_addr->sin_addr.s_addr = INADDR_ANY;     // Listen on all interfaces -> any of the machines IP addresses. INDADDR_ANY resolves to 0.0.0.0
    server_addr->sin_port = htons(port);           // Set port number

    // Bind the socket to the address and port -> reserves this port and IP addr for this socket
    if (bind(serverfd, (struct sockaddr *)server_addr, server_addr_len) < 0)
    {
        perror("\nwelcome socket binding failed");
        close(serverfd);
        return -1;
    }
    return 0;
}

/**
 * @brief Starts listening for incoming connections on the server socket.
 * 
 * @param serverfd The server socket file descriptor.
 * @return 0 on success, -1 on failure.
 */
int start_listening(int serverfd)
{
    // Listen for incoming connections (max 5 in the queue)
    if (listen(serverfd, NUM_CONNECTIONS) < 0)
    {
        perror("\nwelcome socket listening failed");
        close(serverfd);
        return -1;
    }
    return 0;
}

/**
 * @brief Handles incoming client connections on the server socket.
 * 
 * @param serverfd The server socket file descriptor.
 * @param server_addr Pointer to the sockaddr_in structure holding the server address.
 * @param server_addr_len The length of the server_addr structure.
 * @return 0 on success, -1 on failure.
 */
int handle_client(int serverfd, struct sockaddr_in *server_addr,
    socklen_t server_addr_len)
{
    int incoming_socketfd;
    char buffer[BUFFER_SIZE] = {0}; // buffer for incoming data from clients

    if (accept_client(serverfd, &incoming_socketfd, server_addr, &server_addr_len) < 0)
    {
        close(serverfd);
        return -1;
    }

    if (recieve_message(incoming_socketfd, buffer) < 0)
    {
        close(incoming_socketfd);
        close(serverfd);
        return -1;
    }

    close(incoming_socketfd);

    //socket_to_string(serverfd, server_addr); // TESTING
    
    return 0;
}

/**
 * @brief Accepts an incoming client connection on the server socket.
 * 
 * @param serverfd The server socket file descriptor.
 * @param clientfd Pointer to an integer where the accepted client socket file descriptor will be stored.
 * @param server_addr Pointer to the sockaddr_in structure holding the server address.
 * @param server_addr_len Pointer to the length of the server_addr structure.
 * @return 0 on success, -1 on failure.
 */
int accept_client(int serverfd, int *clientfd, struct sockaddr_in *server_addr,
    socklen_t *server_addr_len)
{
    // error check for null pointers
    if (clientfd == NULL || server_addr == NULL || server_addr_len == NULL) {
        fprintf(stderr, "accept_client: invalid arguments\n");
        return -1;
    }
    
    // creating a new socket for an incoming ping. program WAITS for incoming request
    // accept() creates new socket specifically for this client
    if ((*clientfd = accept(serverfd, (struct sockaddr*)server_addr,
        server_addr_len)) < 0)
    { 
        perror("accept failed");
        return -1;
    }
    return 0;
}   

/**
 * @brief Receives a message from the client socket.
 * 
 * @param clientfd The client socket file descriptor.
 * @param buffer Pointer to the buffer where the received message will be stored.
 * @return The number of bytes received on success, -1 on failure.
 */
ssize_t recieve_message(int clientfd, char *buffer)
{
    ssize_t bytes_read; // amnt of bytes read from client

    bytes_read = recv(clientfd, buffer, BUFFER_SIZE - 1, 0);
    if(bytes_read < 0)
    {
        perror("recv failed");
        return -1;
    } else if (bytes_read == 0)
    {
        printf("client disconnected");
        return 0;
    } else
    {
        // null terminate what's in buffer so we can treat it as a c-string
        buffer[bytes_read] = '\0';

        // handleRequest() will replace parseRequest
        parseRequest(buffer);

        //printf("client message: \n%s\n", buffer);
    }
    return bytes_read;
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
void *worker_function(void *arg)
{
    while (1)
    {
        sem_wait(&sem_items); // Puts process to sleep until there's at least one item. Will be posted by a producer?
        sem_wait(&sem_q);     // Puts process to sleep if the critical section is being accessed by another process

        int clientSocket = deq();

        sem_post(&sem_q); // Unlock access to the q

        handle_request(clientSocket);
        close(clientSocket);
    }
}
 // will parse the http request in buffer and populate HTTPRequest and HTTPHeader
void parseRequest(char *buffer){
    const char rq_delims[] = "\n";
    const char start_delims[] = " ";
    char *token = strtok(buffer, rq_delims); //a pointer to the first token in the array
    char tokens[1024];

   while (token != NULL){
    printf("%s\n", token);
    token = strtok(NULL, rq_delims);
   }

}

int deq()
{
    return 0;
}

void handle_request()
{
    ;
}
