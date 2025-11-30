/**
 * Summary: Simple TCP server 'main' function to set up sockets
 *
 * @file server.c
 * @authors: Anna Running Rabbit, Joseph Mills, Jordan Senko
 */

#include "server.h"
#include "common.h"
#include <sys/stat.h>
#include <signal.h>

// --- THREADING GLOBALS ---
pthread_t thread_pool_ids[NUM_THREADS]; 
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER; // The Lock
pthread_cond_t queue_cond_var = PTHREAD_COND_INITIALIZER; // The "Wake Up" Signal

// The Queue (Circular Buffer)
int socket_queue[MAX_SOCKETS]; // Changed to int! We only need the file descriptor
int queue_head = 0;
int queue_tail = 0;
int queue_count = 0;

/******************************************************REPLACED****************
struct sockaddr_in socket_q[MAX_SOCKETS]; // Queue of sockets provided by the main thread. Sockets consumed by the worker threads
******************************************************************************/
int socket_q[MAX_SOCKETS];          // Queue of clientfile descriptors
sem_t sem_items;                    // Counts number of items in the queue
sem_t sem_spaces;                   // Number of available spaces in queue
pthread_mutex_t q_mutex;            // Lock protecting critical sections
int q_head = 0;                     // Consumer - where to take items
int q_tail = 0;                     // Producer - where to add items

// Common syscalls:
// socket(), bind(), connect(), recv(), send(), accept()

// You need this for the signal function
#include <signal.h> 

int main()
{
    // Prevent crashes if a client disconnects abruptly
    signal(SIGPIPE, SIG_IGN); 

    // Start the Worker Threads
    thread_pool(); 
    
    // Setup the Server Port
    int serverfd = welcome_socket(PORT); 
    if (serverfd < 0) {
        return -1;
    }
    printf("Server listening on port %d...\n", PORT);

    // Accept -> Enqueue -> Repeat all day long
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    while (1) 
    {
        int client_socket;
        // Wait here until a client connects
        client_socket = accept(serverfd, (struct sockaddr *)&client_addr, &client_len);
        
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }
        // Send the ID to the queue
        enqueue(client_socket);
    }

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
    if (set_socket_opt(serverfd) < 0){
        close(serverfd);
        return -1;
    }

    if (bind_socket(serverfd, port, &server_addr, server_addr_len) < 0){
        close(serverfd);
        return -1;
    }

    if (start_listening(serverfd) < 0){
        close(serverfd);
        return -1;
    }
    // JD
    // if (handle_client(serverfd, &server_addr, server_addr_len) < 0)
    // {
    //     close(serverfd);
    //     return -1;
    // }

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
        if (recieve_message(clientfd, buffer) < 0) {
             // Error logging handled in recieve_message
        }
        close(clientfd);
    }
    return 0;
}

/**
 * @brief Deletes all headers in the HTTPHeader hash table.
 *
 * @param headers Pointer to the pointer of the head of the HTTPHeader hash table.
 */
void delete_all_headers(HTTPHeader **headers)
{
    HTTPHeader *current_header, *tmp;

    HASH_ITER(hh, *headers, current_header, tmp)
    {
        HASH_DEL(*headers, current_header);
        free(current_header);
    }
}

/**
 * @brief Adds a header key-value pair to the HTTPHeader hash table.
 *
 * @param headers Pointer to the pointer of the head of the HTTPHeader hash table.
 * @param key The header name (e.g., "Host").
 * @param value The header value (e.g., "www.example.com").
 */
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

/**
 * @brief Parse the http request in buffer and populate HTTPRequest and
 *        HTTPHeader structures.
 *
 * @param buffer Pointer to the buffer containing the HTTP request.
 * @param rq Pointer to the HTTPRequest structure to be populated.
 */
// void parse_request(const char *buffer, HTTPRequest *rq)
int parse_request(const char *buffer, HTTPRequest *rq)
{
    char *buffer_copy = strdup(buffer); // make a modifiable copy of buffer;
    if (buffer_copy == NULL)
    {
        perror("strdup failed");
        // return;
        return 500; // internal server error
    }

    char *line_token, *saveptr_line;

    line_token = strtok_r(buffer_copy, "\r\n", &saveptr_line); // get first line

    // Check for empty request
    if (line_token == NULL)
    {
        fprintf(stderr, "malformed request: empty or missing request line.\n");
        free(buffer_copy);
        // return;
        return 400; // bad request
    }

    // Parse request line
    if (parse_request_line(line_token, rq) != 0)
    {
        free(buffer_copy);
        return 400; // bad request
    }

    // Parse headers
    while ((line_token = strtok_r(NULL, "\r\n", &saveptr_line)) != NULL)
    {
        parse_single_header(line_token, rq);
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
    free(buffer_copy); // Free the writable copy
    return 200; // Ok
}

/**
 * @brief Parses the request line of the HTTP request.
 *
 * @param line The request line to be parsed.
 * @param rq Pointer to the HTTPRequest structure to be populated.
 *
 * @return 0 on success, -1 on failure.
 */
int parse_request_line(const char *line, HTTPRequest *rq)
{
    if (sscanf(line, "%9s%1023s%9s", rq->method, rq->path, rq->version) != 3)
    {
        fprintf(stderr, "malformed request line: %s\n", line);
        return -1;
    }
    return 0;
}

/**
 * @brief Parses a single HTTP header line and adds it to the HTTPRequest structure.
 *
 * @param line The header line to be parsed.
 * @param rq Pointer to the HTTPRequest structure to which the header will be added.
 */
void parse_single_header(const char *line, HTTPRequest *rq)
{
    char *key = line;

    // searches for first occurence of ":" in line_token and returns pointer to
    // location in array
    char *value = strchr(line, ':');

    if (value)
    {
        *value = '\0'; // null terminate the key
        value++;       // move past the colon to the start of the value

        add_header_to_hash(&rq->headers, key, value);
    }
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
    /*
    MUST be initialized to NULL to indicate an empty hash table.
    When adding headers, we will use uthash macros which will handle
    the hash table management for us.
    */
    rq.headers = NULL; // ptr to head of HTTPHeader hash table
    
    int status = parse_request(buffer, &rq); // parse client request 
    // error check
    if (status != 200)
    {
        send_error_response("Request Parsing", clientfd, status);
        return;
    }

    // double the PATH_LEN to accommodate full file paths without overflow risk
    char filepath[PATH_LEN * 2];
    create_root_path(filepath, &rq);

    // error check for bad request
    if (strcmp(filepath, "invalid_path") == 0)
    {
        send_error_response(filepath, clientfd, 400);
        return;
    }

    struct stat file_stat; // will contain info about the file

    // If file doesn't exist
    if (stat(filepath, &file_stat) < 0)
    {
        // writes states about what's at filepath to filestat
        send_error_response(filepath, clientfd, 404);
    }
    // If file exists
    else
    {
        serve_file(clientfd, filepath, file_stat.st_size);
    }
    delete_all_headers(&rq.headers); // clean up allocated hash table memory
    // JD close(clientfd);                 // might not be needed
}

/**
 * @brief Creates the full file path for the requested resource.
 *
 * @param filepath Pointer to the buffer where the full file path will be stored.
 * @param rq Pointer to the HTTPRequest structure containing the request details.
 */
void create_root_path(char *filepath, HTTPRequest *rq)
{
    // Security check to block access to www parent folders
    if (strstr(rq->path, "..") != NULL)
    {
        sprintf(filepath, "invalid_path");
        return;
    }
    
    // create root directory path so source files are seperate from server files
    if (strcmp(rq->path, "/") == 0)
    {
        sprintf(filepath, "www/index.html");

        printf("Handling request for path: %s\n", rq->path);
    } // construct full file path
    else
    {
        sprintf(filepath, "www%s", rq->path);
    }
}

/**
 * @brief Sends an error response to the client based on the status code.
 *
 * @param filepath The file path related to the error.
 * @param clientfd The client socket file descriptor.
 * @param status_code The HTTP status code indicating the type of error.
 */
void send_error_response(const char *filepath, int clientfd, int status_code)
{
    char response[256];

    if (status_code == 400)
    {
         // print error message to server console
        printf("Error 400: Bad Request: %s\n", filepath);

        // create response for client
        sprintf(response, "HTTP/1.1 400 Bad Request\r\n");       
    }
    else if (status_code == 404)
    {
        // print error message to server console
        printf("Error 404: File not found: %s\n", filepath);

        // create response for client
        sprintf(response, "HTTP/1.1 404 Not Found\r\n");
    }
    else
    {
        // print error message to server console
        printf("Error 500: Internal Server Error for file: %s\n", filepath);

        // create response for client
        sprintf(response, "HTTP/1.1 500 Internal Server Error\r\n");
    }
    send(clientfd, response, strlen(response), 0);
}

/**
 * @brief Serves the requested file to the client.
 *
 * @param clientfd The client socket file descriptor.
 * @param filepath The path of the file to be served.
 * @param filesize The size of the file in bytes.
 */
void serve_file(int clientfd, const char *filepath, off_t filesize)
{
    FILE *file = fopen(filepath, "rb");

    if (file == NULL)
    {
        printf("Failed to open file: %s\n", filepath);
        send_error_response(filepath, clientfd, 500);
        return;
    }

    const char *mime_type = get_mime_type(filepath);
    char header[PATH_LEN];

    char *file_name = strchr(filepath, '/');

    // Build header
    sprintf(header, "HTTP/1.1 200 OK\r\n"
                    "File-Name: %s\r\n"
                    "Content-Length: %ld\r\n"
                    "Content-Type: %s\r\n"
                    "\r\n",
            file_name, filesize, mime_type);

    // Send Header
    if (send(clientfd, header, strlen(header), 0) == -1)
    {
        perror("Failed to send header");
        fclose(file);
        return;
    }

    // Send Body
    char file_buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(file_buffer, 1, sizeof(file_buffer), file)) > 0)
    {
        if (send(clientfd, file_buffer, bytes_read, 0) == -1)
        {
            printf("Failed to send file content");
            send_error_response(filepath, clientfd, 500);
            break;
        }
    }

    fclose(file);
    printf("Served file: %s\n", filepath);
}

/**
 * @brief Determines the MIME type based on the file extension.
 *
 * @param filepath The path of the file.
 * @return A string representing the MIME type.
 */
const char *get_mime_type(const char *filepath)
{
    const char *ext = strrchr(filepath, '.'); // find last occurrence of '.'

    if (ext == NULL)
    {
        return "application/octet-stream"; // default for unknown/no extension
    }

    // skip the dot
    ext++;

    if (strcmp(ext, "html") == 0 || strcmp(ext, "htm") == 0)
    {
        return "text/html";
    }
    else if (strcmp(ext, "css") == 0)
    {
        return "text/css";
    }
    else if (strcmp(ext, "js") == 0)
    {
        return "application/javascript";
    }
    else if (strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0)
    {
        return "image/jpeg";
    }
    else if (strcmp(ext, "png") == 0)
    {
        return "image/png";
    }
    else if (strcmp(ext, "gif") == 0)
    {
        return "image/gif";
    }
    else if (strcmp(ext, "json") == 0)
    {
        return "application/json";
    }
    // Add more types as needed
    return "application/octet-stream"; // Fallback
}

/**
 * @brief Initializes the thread pool by creating worker threads.
 */
void thread_pool(){
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_create(&thread_pool_ids[i], NULL, worker_function, NULL);
        printf("made a thread\n", NUM_THREADS);
    }
    printf("Thread pool initialized with %d workers.\n", NUM_THREADS);
}

/**
 * @brief Enqueues a client socket into the socket queue for processing by worker threads.
 *
 * @param client_socket The client socket file descriptor to be enqueued.
 */
void enqueue(int client_socket) {
    pthread_mutex_lock(&queue_mutex); // Thou shalt not unlock

    // Check if queue is full
    if (queue_count < MAX_SOCKETS) 
    {
        socket_queue[queue_tail] = client_socket; // Put ticket in buffer
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
    printf("  [Worker %lu] Dequeued Client %d. (Queue Remaining: %d)\n", pthread_self(), client_socket, queue_count);
    pthread_mutex_unlock(&queue_mutex); // Unlock the door
    return client_socket;
}