/**
 * Summary: Main client application for initiating HTTP requests and handling connection lifecycle.
 *
 * @file client.c
 * @authors: Anna Running Rabbit, Joseph Mills, Jordan Senko
 */
#include "c_http_parser.h"

#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>

#define PORT 6767

// --- FUNCTION DECLERATIONS ---
int client_socket(uint16_t port, struct sockaddr_in server_addr);
int connect_client(int clientfd, struct sockaddr_in server_addr);
int content_length(const char *buffer);

// -- FUNCTIONS ---
/**
 * @brief Main entry point for the client application.
 *        Facilitates the client lifecycle:
 *        1. Defines the hardcoded HTTP request string (Method, Path, Host)
 *        2. Initializes the socket connection via client_socket
 *        3. Sends the request and awaits the response via send_request()
 *        4. Logs process start and finish times using the process ID (PID) for tracing
 * @return 0 on successful execution, -1 if socket creation fails.
 */
int main()
{
    pid_t pid = getpid();
    printf("[PID %d] - client process started.\n", pid);

    // the "\r\n\r\n" sequence signals the end of the request header block.
    // client is only responsible for sending over bytes. server must parse message once recieved
    const char message[] =
        "GET /index.html HTTP/1.1\r\n"
        //"GET /shrek-rizz.gif HTTP/1.1\r\n"
        // "GET /thomas.JPG HTTP/1.1\r\n"
        // "GET /HTTPSlides.png HTTP/1.1\r\n"
        //"GET /A6.pdf HTTP/1.1\r\n"                
        // "GET /PP2_Concept_Memo.pdf HTTP/1.1\r\n"
        // "GET /paintings-nested.json HTTP/1.1\r\n"
        // "GET /api.js HTTP/1.1\r\n"
        // "GET /product.css HTTP/1.1\r\n"
        "Host: 127.0.0.1:6767\r\n"
        "Connection: close\r\n"
        "\r\n";

    struct sockaddr_in server_addr;
    int serverfd = client_socket(PORT, server_addr);
    if (serverfd < 0)
    {
        return -1;
    }
    send_request(serverfd, message);

    printf("[PID %i] - client process finished.\n", pid);
    return 0;
}

/**
 * @brief Creates and configures a TCP client socket.
 *        Creates a socket file descriptor using IPv4 and TCP (SOCK_STREAM).
 *        It populates the server_addr structure with the target family, port, and IP address.
 *        It also converts the string IP ("127.0.0.1") to binary network format using inet_pton().
 *        It attempts to connect to the server using connect_client().
 *
 * @param port Port number to connect to (host byte order)
 * @param server_addr sockaddr_in structure to be populated with server details
 * @return Socket file descriptor on success, or -1 on failure
 */
int client_socket(uint16_t port, struct sockaddr_in server_addr)
{
    pid_t pid = getpid();

    int clientfd;
    clientfd = socket(AF_INET, SOCK_STREAM, 0); // creating client socket using IPv4 and TCP

    if (clientfd < 0)
    {
        printf("[PID %i] - ❌ (1/5) client socket creation failed\n", pid);
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // convert IPv4 and IPv6 into binary for error checking
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0)
    {
        printf("[PID %i] - ❌ (1/5) socket hosted on invalid/unsupported address\n", pid);
        return -1;
    }

    printf("[PID %i] - ✔️ (1/5) client socket creation successful\n", pid);

    if (connect_client(clientfd, server_addr) != 0)
    {
        return -1;
    }

    return clientfd;
}

/**
 * @brief Establishes a connection to the server using the configured socket.
 *        This function calls the connect() syscall to initiate the TCP 3-way handshake
 *        with the server specified in the server_addr structure. It provides logging
 *        for successful or failed connection attempts.
 * @param clientfd The socket file descriptor to connect
 * @param server_addr Structure containing the server's IP and port
 * @return 0 on success, -1 on failure.
 */
int connect_client(int clientfd, struct sockaddr_in server_addr)
{
    pid_t pid = getpid();

    int status = connect(clientfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (status < 0)
    {
        printf("[PID %i] - ❌ (2/5) connection failed\n", pid);
        return -1;
    }
    printf("[PID %i] - ✔️ (2/5) client connected successfuly\n", pid);
    return 0;
}

