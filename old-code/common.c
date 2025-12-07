#include "common.h"

// Function for testing. Shows the file descriptor, address, and port of a socket
int socket_to_string(int sockfd, struct sockaddr_in server_addr)
{
    // Convert the port from Network Byte Order to Host Byte Order for printing
    char addr_str[20]; // for TESTING only
    struct sockaddr_in actual_addr;
    socklen_t actual_addr_len = sizeof(actual_addr);

    if (getsockname(sockfd, (struct sockaddr *)&actual_addr, &actual_addr_len) == 0)
    {
        // Now, actual_addr.sin_port holds the *correct* port (in network byte order)
        uint16_t actual_port = ntohs(actual_addr.sin_port); // converts from NETWORK BYTE ORDER to HOST BYTE ORDER
    }
    else
    {
        perror("getsockname failed");
    }

    // Convert the binary IP address to a string. FOR TESTING ONLY
    // NOTE: This now gets the IP from the *actual_addr* structure if you want the IP bound to the socket.
    if (inet_ntop(AF_INET, &actual_addr.sin_addr, addr_str, sizeof(addr_str)) == NULL)
    {
        perror("inet_ntop");
        return 1;
    }

    uint16_t display_port = ntohs(server_addr.sin_port); // converts NETWORK BYTE ORDER to HOST BYTE ORDER

    printf("SocketFD: %i\n", sockfd);
    printf("Address (IPv4): %s\n", addr_str);
    printf("Port: %i\n", display_port); // Print the converted port
    return 0;
}