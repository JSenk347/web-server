#include "common.h"

#define FILE_NAME_LEN 32

void send_request(int sockfd, const char request[]);
int connect_client(int clientfd, struct sockaddr_in server_addr);

int main(int argc, char *argv[])
{

    // the "\r\n\r\n" sequence signals the end of the request header block.
    // client is only responsible for sending over bytes. server must parse message once recieved
    const char message[] =
        "GET www/HTTPSlides.png HTTP/1.1\r\n"
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

    return 0;
}

int client_socket(uint16_t port, struct sockaddr_in server_addr)
{
    int clientfd;
    clientfd = socket(AF_INET, SOCK_STREAM, 0); // creating client socket using IPv4 and TCP

    if (clientfd < 0)
    {
        perror("    - client socket creation failed\n");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // convert IPv4 and IPv6 into binary for error checking
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0)
    {
        perror("    - invalid address / address not supported\n");
        return -1;
    }

    if (connect_client(clientfd, server_addr) != 0)
    {
        perror("    - connection failed\n\n");
    }

    // socket_to_string(clientfd, server_addr); // TESTING
    return clientfd;
}

int connect_client(int clientfd, struct sockaddr_in server_addr)
{
    printf("----> Connection Establishment\n");
    int status = connect(clientfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (status < 0)
    {
        perror("    - connection failed\n\n");
        return -1;
    }
    printf("    - client connected successfuly\n\n");
    return 0;
}

void print_response(char *buffer)
{
    printf("%s\n", buffer);
}

void read_remaining_body_bytes(long remaining_bytes, long *total_written, char *buffer, int sockfd, FILE *outfile, char *file_name)
{
    size_t bytes_recieved = -1;

    while (remaining_bytes > 0)
    {
        size_t bytes_to_recieve = (remaining_bytes > sizeof(buffer)) ? sizeof(buffer) : remaining_bytes;
        bytes_recieved = recv(sockfd, buffer, bytes_to_recieve, 0);

        if (bytes_recieved < 0)
        {
            perror("            - Warning: Premature end of body data");
            return;
        }
        // fwrite(ptr_to_data_to_write, size_to_be_written, num_elems_to_write, ptr_to_file)
        // using size_to_be_written = 0 because it is raw binary data
        fwrite(buffer, 1, bytes_recieved, outfile);
        remaining_bytes -= bytes_recieved;
        *total_written += bytes_recieved;
    }
    printf("            - wrote %li bytes to %s\n\n", *total_written, file_name);
}

void save_file(size_t body_bytes, char *body_start, int content_len, char *file_name, int sockfd)
{
    printf("    ----> Saving File\n");
    FILE *outfile = NULL;
    char file_path[FILE_NAME_LEN]; // buffer for "client_req/filename"
    char buffer[BUFFER_SIZE];      // local buffer for receiving data

    file_name++; // to skip past "/"

    snprintf(file_path, sizeof(file_path), "client_req/%s", file_name);
    printf("            - file path: %s\n", file_path);

    outfile = fopen(file_path, "wb"); // open file in (wb) write binary mode
    if (outfile == NULL)
    {
        perror("            - Error: Failed to open file for writing\n\n");
        return;
    }

    if (body_bytes > 0)
    {
        fwrite(body_start, 1, body_bytes, outfile);
    }

    long remaining_bytes = content_len - body_bytes;
    long long_body_bytes = body_bytes;
    read_remaining_body_bytes(remaining_bytes, &long_body_bytes, buffer, sockfd, outfile, file_name);

    fclose(outfile);
}

int content_length(const char *buffer)
{
    printf("    ----> Extracting Content Length\n");
    int content_length = -1;
    char *len_line = strstr(buffer, "Content-Length:");

    if (len_line)
    {
        if (sscanf(len_line, "Content-Length: %d", &content_length) == 1)
        {
            printf("            - content length: %i bytes\n\n", content_length);
            return content_length;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        printf("            - Warning: Content-Length not found. Cannot reliably read file body.\n\n");
        return -1;
    }
}

/**
 * @brief Safely extracts a header value (case-insensitive) from the header buffer.
 * @return Pointer to output_buffer on success, NULL on failure.
 */
char *get_header_value(const char *buffer, const char *header_key, char *output_buffer, size_t output_size)
{
    char search_key[64];
    char *start, *end, *newline_pos;

    // create the search string e.g., "File-Name:"
    snprintf(search_key, sizeof(search_key), "%s:", header_key);

    // find proper key
    start = strstr(buffer, search_key);
    if (!start)
        return NULL;

    // move past key and colon e.g., past "File-Name:"
    start += strlen(search_key);

    // trim leading whitespace
    while (*start == ' ' || *start == '\t')
    {
        start++;
    }

    // find the end of value, the next \r\n
    newline_pos = strstr(start, "\r\n");
    if (!newline_pos)
        return NULL;

    end = newline_pos;

    // copy extracted value into the output buffer
    size_t len = end - start;
    if (len >= output_size)
    {
        len = output_size - 1;
    }

    strncpy(output_buffer, start, len);
    output_buffer[len] = '\0'; // Null-terminate

    return output_buffer;
}

void recieve_response(int serverfd, char *buffer)
{
    printf("----> Recieving Response\n");
    ssize_t bytes_read; // amount of bytes read from client
    size_t total_recieved = 0;
    char header_buffer[BUFFER_SIZE];
    char *body_start = NULL;
    char *header_end = "\r\n\r\n";

    char file_name_output[FILE_NAME_LEN];
    const char *default_file_name = "received_file.bin";

    while (total_recieved < sizeof(header_buffer) - 1)
    {
        // "+ total_recieved" to move to correct location in header_buffer
        bytes_read = recv(serverfd, header_buffer + total_recieved, 1, 0); // reading in byte-by-byte

        if (bytes_read <= 0)
        {
            perror("    - error or premature disconnect during header read");
            return;
        }

        total_recieved += bytes_read;
        header_buffer[total_recieved] = '\0';

        if ((body_start = strstr(header_buffer, header_end)) != NULL)
        {
            break; // have read the last header
        }
    }

    if (body_start != NULL)
    {
        *body_start = '\0'; // null termiante body start for easy reading
        int content_len = content_length(header_buffer);
        if (content_len < 0)
        {
            perror("    - Error: Could not determine Content-Length. Cannot reliably save file body\n");
            return;
        }

        if (!get_header_value(header_buffer, "File-Name", file_name_output, sizeof(file_name_output)))
        {
            printf("    - Warning: 'File-Name' header not found. Using default filename: %s\n", default_file_name);
            strncpy(file_name_output, default_file_name, sizeof(file_name_output));
        }

        body_start += strlen(header_end);

        size_t body_bytes_in_buff = total_recieved - (body_start - header_buffer);

        save_file(body_bytes_in_buff, body_start, content_len, file_name_output, serverfd);
    }
    else
    {
        printf("    - Failed to find end of HTTP headers (\\r\\n\\r\\n)");
    }

    // while ((bytes_read = recv(serverfd, buffer, BUFFER_SIZE - 1, 0)) > 0){
    //     buffer[bytes_read] = '\0';
    //     print_response(buffer);
    // }

    // if (bytes_read < 0){
    //     perror("recv failed");
    // }

    // must write response to appropriate file type and save to client_req
}

void send_request(int sockfd, const char request[])
{
    printf("----> Sending HTTP Request\n");
    size_t len = strlen(request);
    ssize_t bytes_sent;
    char resp_buffer[BUFFER_SIZE];

    bytes_sent = send(sockfd, request, len, 0);

    // check for errors and handle partial sends
    if (bytes_sent == (ssize_t)len)
    {
        printf("    - message sent (%zd bytes)\n\n", bytes_sent);
    }
    else if (bytes_sent < 0)
    {
        perror("    - error sending message\n");
    }
    else
    {
        // if bytes_sent > 0 but less than len, a partial send occurred
        printf("    - warning: only sent %zd of %zu bytes.\n\n", bytes_sent, len);
    }

    recieve_response(sockfd, resp_buffer);

    close(sockfd);
}
