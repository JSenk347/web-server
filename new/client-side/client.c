#include "c_http_parser.h"

#define PORT 6767

int main(int argc, char *argv[])
{
    pid_t pid = getpid();
    printf("[PID %d] - client process started.\n", pid);

    // the "\r\n\r\n" sequence signals the end of the request header block.
    // client is only responsible for sending over bytes. server must parse message once recieved
    const char message[] =
        //"GET www/HTTPSlides.png HTTP/1.1\r\n" the www should be handled on the server side ARR
        "GET /A6.pdf HTTP/1.1\r\n"
        "Host: 127.0.0.1:6767\r\n"
        "Connection: close\r\n"
        "\r\n";

    struct sockaddr_in server_addr;
    int clientfd = client_socket(PORT, server_addr);
    if (clientfd < 0)
    {
        return -1;
    }
    send_request(clientfd, message);

    printf("[PID %i] - client process finished.\n", pid);
    return 0;
}

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

int save_file(size_t body_bytes, char *body_start, int content_len, char *file_name, int sockfd)
{
    pid_t pid = getpid();

    FILE *outfile = NULL;
    char file_path[FILE_NAME_LEN]; // buffer for "client_req/filename"
    char buffer[BUFFER_SIZE];      // local buffer for receiving data

    file_name++; // to skip past "/"

    snprintf(file_path, sizeof(file_path), "client_req/%s", file_name);

    outfile = fopen(file_path, "wb"); // open file in (wb) write binary mode
    if (outfile == NULL)
    {
        printf("[PID %i] - ❌ (5/5) Error: failed to open file for writing\n", pid);
        return -1;
    }

    if (body_bytes > 0)
    {
        fwrite(body_start, 1, body_bytes, outfile);
    }

    printf("[PID %i] - ✔️ (5/5) %i bytes written to %s, saved in %s\n", pid, content_len, file_name, file_path);

    long remaining_bytes = content_len - body_bytes;
    long long_body_bytes = body_bytes;
    read_remaining_body_bytes(remaining_bytes, &long_body_bytes, buffer, sockfd, outfile, file_name);

    fclose(outfile);
    return 0;
}

int content_length(const char *buffer)
{
    pid_t pid = getpid();

    int content_length = -1;
    char *len_line = strstr(buffer, "Content-Length:");

    if (len_line)
    {
        if (sscanf(len_line, "Content-Length: %d", &content_length) == 1)
        {
            return content_length;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        printf("[PID %i] - ⚠️ Warning: Content-Length not found. cannot reliably read file body.\n", pid);
        return -1;
    }
}