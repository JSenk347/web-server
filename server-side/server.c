/**
 * Summary: Main server entry point, responsible for socket creation, binding, listening, and the main accept loop.
 *
 * @file server.c
 * @authors: Anna Running Rabbit, Joseph Mills, Jordan Senko
 */
#include "thread_pool.h"
#include "http_parser.h"

#include <signal.h>
#include <netdb.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define PORT 6767
#define NUM_CONNECTIONS 5

// --- FUNCTION DECLERATIONS ---
int welcome_socket(uint16_t port);
int create_socket(int *socketfd, int domain, int type);
int set_socket_opt(int serverfd);
int bind_socket(int serverfd, uint16_t port, struct sockaddr_in *server_addr,
                socklen_t server_addr_len);
int start_listening(int serverfd);

// --- FUNCTIONS ---
int main()
{
    // prevent crashes if a client disconnects abruptly
    signal(SIGPIPE, SIG_IGN);

    // start the worker threads
    thread_pool();

    // setup the server port
    int serverfd = welcome_socket(PORT);
    if (serverfd < 0)
    {
        return -1;
    }
    printf(" - ✔️ Server listening on port %d...\n", PORT);

    // accept -> enqueue -> repeat all day long
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (1)
    {
        int client_socket;
        // accept is a blocking function that will block until a client connects
        client_socket = accept(serverfd, (struct sockaddr *)&client_addr, &client_len);

        if (client_socket < 0)
        {
            printf(" - ❌ Error: accept() failed\n");
            continue;
        }
        // send the ID to the queue
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

    return serverfd; // file descriptor of the socket
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
    // validate input pointer
    if (socketfd == NULL)
    {
        fprintf(stderr, " - ❌ Error: invalid pointer passed to create_wel_socket\n");
        return -1;
    }

    // create the socket
    *socketfd = socket(/*AF_INET*/ domain, /*SOCK_STREAM*/ type, 0);

    // error check socket creation failure
    if (*socketfd < 0)
    {
        perror(" - ❌ Error: welcome socket creation failed\n");
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
    // set address/port reusable (optional, for quick restarts)
    int opt = 1;

    if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        // eror check setsockopt failure
        perror(" - ❌ Error: setsockopt failed\n");
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
       with 0 for sizeof(server_addr) bytes 
    */
    memset(server_addr, 0, server_addr_len);

    server_addr->sin_family = AF_INET;         // specifies the server address TYPE to IPv4
    server_addr->sin_addr.s_addr = INADDR_ANY; // listen on all interfaces -> any of the machines IP addresses. INDADDR_ANY resolves to 0.0.0.0
    server_addr->sin_port = htons(port);       // set port number

    // bind the socket to the address and port -> reserves this port and IP addr for this socket
    if (bind(serverfd, (struct sockaddr *)server_addr, server_addr_len) < 0)
    {
        perror(" - ❌ Error: welcome socket binding failed");
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
    // listen for incoming connections
    if (listen(serverfd, NUM_CONNECTIONS) < 0)
    {
        perror(" - ❌ Error: welcome socket listening failed\n");
        close(serverfd);
        return -1;
    }
    return 0;
}