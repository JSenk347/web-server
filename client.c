#include "common.h"

void send_request(int sockfd, const char request[]);
int connect_client(int clientfd, struct sockaddr_in server_addr);

int main(int argc, char *argv[])
{
    const char message[] = "What is up my guy!?";
    struct sockaddr_in server_addr;
    int clientfd = client_socket(PORT, server_addr);
    if (clientfd < 0){
        return -1;
    }
    send_request(clientfd, message);

    return 0;
}

int client_socket(uint16_t port, struct sockaddr_in server_addr)
{
    int clientfd;
    clientfd = socket(AF_INET, SOCK_STREAM, 0); // creating client socket using IPv4 and TCP

    if (clientfd < 0)
    { 
        perror("\nclient socket creation failed");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // convert IPv4 and IPv6 into binary for error checking
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0)
    {
        perror("\ninvalid address / address not supported");
        return -1;
    }

    if (connect_client(clientfd, server_addr) != 0){
        perror("connection failed\n");
    }

    //socket_to_string(clientfd, server_addr); // TESTING
    return clientfd;
}

int connect_client(int clientfd, struct sockaddr_in server_addr){
    int status = connect(clientfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (status < 0)
    {
        perror("connection failed\n");
        return -1;
    } 
    printf("client connected successfuly\n");
    return 0;
}

void send_request(int sockfd, const char request[]){
    size_t len = strlen(request);
    ssize_t bytes_sent;

    bytes_sent = send(sockfd, request, len, 0);

    // check for errors and handle partial sends
    if (bytes_sent == (ssize_t)len) {
        printf("message sent (%zd bytes)\n", bytes_sent);
    } else if (bytes_sent < 0) {
        perror("error sending message");
    } else {
        // if bytes_sent > 0 but less than len, a partial send occurred
        printf("warning: only sent %zd of %zu bytes.\n", bytes_sent, len);
    }

    close(sockfd);
}
