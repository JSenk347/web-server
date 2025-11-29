/**
 * Summary: Simple TCP server 'main' function to set up sockets
 *
 * @file server.c
 * @authors: Anna Running Rabbit, Joseph Mills, Jordan Senko
 */

#include "server.h"
#include "common.h"
#include <sys/stat.h>

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
    if (create_socket(&serverfd, AF_INET, SOCK_STREAM) < 0)
        return -1;

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
    *socketfd = socket(/*AF_INET*/ domain, /*SOCK_STREAM*/ type, 0);

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

    server_addr->sin_family = AF_INET;         // Specifies the server address TYPE to IPv4
    server_addr->sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces -> any of the machines IP addresses. INDADDR_ANY resolves to 0.0.0.0
    server_addr->sin_port = htons(port);       // Set port number

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

    // socket_to_string(serverfd, server_addr); // TESTING

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
    if (clientfd == NULL || server_addr == NULL || server_addr_len == NULL)
    {
        fprintf(stderr, "accept_client: invalid arguments\n");
        return -1;
    }

    // creating a new socket for an incoming ping. program WAITS for incoming request
    // accept() creates new socket specifically for this client
    if ((*clientfd = accept(serverfd, (struct sockaddr *)server_addr,
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
    if (bytes_read < 0)
    {
        perror("recv failed");
        return -1;
    }
    else if (bytes_read == 0)
    {
        printf("client disconnected");
        return 0;
    }
    else
    {
        // null terminate what's in buffer so we can treat it as a c-string
        buffer[bytes_read] = '\0';

        // handleRequest() will replace parseRequest
        // parse_request(buffer);
        handle_request(clientfd, buffer);

        // printf("client message: \n%s\n", buffer);
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
        /******COMMENTED OUT FOR TESTING, WILL ADD BACK ONCE THREADS ARE IMPLEMENTED ****************
        sem_wait(&sem_items); // Puts process to sleep until there's at least one item. Will be posted by a producer?
        sem_wait(&sem_q);     // Puts process to sleep if the critical section is being accessed by another process

        int clientSocket = deq();

        sem_post(&sem_q); // Unlock access to the q

        handle_request(clientSocket, );
        close(clientSocket);
        *******************************************************************************************/
    }
}

void delete_all_headers(HTTPHeader **headers)
{
    HTTPHeader *current_header, *tmp;

    HASH_ITER(hh, *headers, current_header, tmp)
    {
        HASH_DEL(*headers, current_header);
        free(current_header);
    }
}

void add_header_to_hash(HTTPHeader **headers, const char *key, const char *value)
{
    HTTPHeader *s = (HTTPHeader *)malloc(sizeof(HTTPHeader));
    if (s == NULL)
    {
        perror("failed to allcoate HTTPHeader on the heap");
        return;
    }

    const char *trimmed_val = value;
    while (*trimmed_val == ' ')
    {
        trimmed_val++;
    }

    strncpy(s->key, key, sizeof(s->key));
    s->key[sizeof(s->key) - 1] = '\0';
    strncpy(s->value, trimmed_val, sizeof(s->value));
    s->value[sizeof(s->value) - 1] = '\0';

    HASH_ADD_STR(*headers, key, s);
}

// will parse the http request in buffer and populate HTTPRequest and HTTPHeader
void parse_request(const char *buffer, HTTPRequest *rq)
{
    // HTTPRequest rq;
    /*
    MUST be initialized to NULL to indicate an empty hash table.
    When adding headers, we will use uthash macros which will handle
    the hash table management for us.
    */
    rq->headers = NULL; // ptr to head of HTTPHeader hash table

    char *buffer_copy = strdup(buffer); // make a modifiable copy of buffer;

    if (buffer_copy == NULL)
    {
        perror("strdup failed");
        return;
    }

    char *line_token, *saveptr_line;

    line_token = strtok_r(buffer_copy, "\r\n", &saveptr_line); // get first line

    if (line_token == NULL)
    {
        fprintf(stderr, "malformed request: empty or missing request line.\n");
        free(buffer_copy);
        return;
    }

    if (sscanf(line_token, "%9s%1023s%9s", rq->method, rq->path, rq->version) != 3)
    {
        fprintf(stderr, "malformed request line: %s\n", line_token);
        free(buffer_copy);
        return;
    }

    while ((line_token = strtok_r(NULL, "\r\n", &saveptr_line)) != NULL)
    {
        if (line_token[0] == '\0')
        {
            break;
        }

        char *key = line_token;
        char *value = strchr(line_token, ':'); // searchers for first occurence of ":" in line_token and returns pointer to location in array

        if (value)
        {
            *value = '\0'; // null terminate the key
            value++;       // move past the colon to the start of the value

            add_header_to_hash(&rq->headers, key, value);
        }
        else
        {
            break;
        }
    }

    printf("\n--- Parsed HTTP Request ---\n");
    printf("Method: %s\n", rq->method);
    printf("Path: %s\n", rq->path);
    printf("Version: %s\n", rq->version);

    printf("Headers:\n");
    HTTPHeader *current_header, *tmp;
    HASH_ITER(hh, rq->headers, current_header, tmp)
    {
        printf(" - %s: %s\n", current_header->key, current_header->value);
    }
    printf("---------------------------\n");

    // Clean up allocated hash table memory
    // delete_all_headers(&rq->headers); now done in handle_request()
    free(buffer_copy); // Free the writable copy
}

int deq()
{
    return 0;
}

/**
 * @brief Parses and handles the requests received from the client.
 *          So far, only handles HTTP requests.
 *
 * @param clientfd The client socket file descriptor.
 * @param buffer Pointer to the buffer containing the received HTTP request.
 */
void handle_request(int clientfd, const char *buffer)
{
    HTTPRequest rq;
    parse_request(buffer, &rq);

    // TODO: Add logic to generate and send HTTP response based on parsed request

    // double the PATH_LEN to accommodate full file paths without overflow risk
    char filepath[PATH_LEN * 2];

    // create root directory path so source files are seperate from server files
    if (strcmp(rq.path, "/") == 0)
    {
        sprintf(filepath, "www/index.html");
        
        printf("Handling request for path: %s\n", rq.path);
    } //
    else
    {
        sprintf(filepath, "www%s", rq.path);
    } // construct full file path

    struct stat file_stat;

    // If file doesn't exist
    if (stat(filepath, &file_stat) < 0)
    {
        // print error message to server console
        printf("File not found: %s\n", filepath);

        // send 404 response to client
        const char *not_found_msg = "HTTP/1.1 404 Not Found\r\n";
        send(clientfd, not_found_msg, strlen(not_found_msg), 0);
    }
    // If file exists
    //else
    //{
        // Serve file??
    //    FILE *file = fopen(filepath, "rb");  // create file pointer to read file in binary mode
    //}

    // send 200 OK response to client
    const char *ok_msg = "HTTP/1.1 200 OK\r\n";
    send(clientfd, ok_msg, strlen(ok_msg), 0);

    // Send file

    delete_all_headers(&rq.headers); // clean up allocated hash table memory
}
