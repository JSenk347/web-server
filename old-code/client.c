#include "common.h"
//#include <unistd.h>

#define FILE_NAME_LEN 32

void send_request(int sockfd, const char request[]);
int connect_client(int clientfd, struct sockaddr_in server_addr);

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

void read_remaining_body_bytes(long remaining_bytes, long *total_written, char *buffer, int sockfd, FILE *outfile, char *file_name)
{
    pid_t pid = getpid();

    size_t bytes_recieved = -1;

    while (remaining_bytes > 0)
    {
        size_t bytes_to_recieve = (remaining_bytes > sizeof(buffer)) ? sizeof(buffer) : remaining_bytes;
        bytes_recieved = recv(sockfd, buffer, bytes_to_recieve, 0);

        if (bytes_recieved < 0)
        {
            printf("[PID %i] - ⚠️ Warning: premature end of body data", pid);
            return;
        }
        // fwrite(ptr_to_data_to_write, size_to_be_written, num_elems_to_write, ptr_to_file)
        // using size_to_be_written = 0 because it is raw binary data
        fwrite(buffer, 1, bytes_recieved, outfile);
        remaining_bytes -= bytes_recieved;
        *total_written += bytes_recieved;
    }
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

int get_status_code(char *buffer)
{
    pid_t pid = getpid();

    int status_code = 0;
    if (sscanf(buffer, "HTTP/1.%*d %d", &status_code) != 1)
    {
        printf("[PID %i] - ❌ Failed to parse HTTP status line.\n", pid);
    }
    return status_code;
}

void recieve_response(int serverfd)
{
    pid_t pid = getpid();

    ssize_t bytes_read = 0; // amount of bytes read from client
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
            printf("[PID %i] - ❌ (4/5) error or premature disconnect during header read\n", pid);

            if (total_recieved > 0)
            {
                // Ensure the buffer is null-terminated and print the raw header data
                header_buffer[total_recieved] = '\0';
                printf("[PID %i] - ⚠️ server response: %s", pid, header_buffer);
            }
            return;
        }

        total_recieved += bytes_read;
        header_buffer[total_recieved] = '\0';

        if ((body_start = strstr(header_buffer, header_end)) != NULL)
        {
            printf("[PID %i] - ✔️ (4/5) recieved HTTP response header\n", pid);
            break; // have read the last header
        }
    }

    if (body_start != NULL)
    {
        *body_start = '\0'; // null termiante body start for easy reading
        int content_len = content_length(header_buffer);
        if (content_len < 0)
        {
            printf("[PID %i] - Error: Could not determine Content-Length. Cannot reliably save file body\n", pid);
            return;
        }

        if (!get_header_value(header_buffer, "File-Name", file_name_output, sizeof(file_name_output)))
        {
            printf("[PID %i] - Warning: 'File-Name' header not found. using default filename: %s\n", pid, default_file_name);
            strncpy(file_name_output, default_file_name, sizeof(file_name_output));
        }

        body_start += strlen(header_end);

        size_t body_bytes_in_buff = total_recieved - (body_start - header_buffer);

        save_file(body_bytes_in_buff, body_start, content_len, file_name_output, serverfd);
    }
    else
    {
        printf("[PID %i] - failed to find end of HTTP headers (\\r\\n\\r\\n)\n", pid);
    }
}

void send_request(int sockfd, const char request[])
{
    pid_t pid = getpid();

    size_t len = strlen(request);
    ssize_t bytes_sent;

    bytes_sent = send(sockfd, request, len, 0);

    // check for errors and handle partial sends
    if (bytes_sent == (ssize_t)len)
    {
        printf("[PID %i] - ✔️ (3/5) message sent (%zd bytes)\n", pid, bytes_sent);
    }
    else if (bytes_sent < 0)
    {
        printf("[PID %i] - ❌ (3/5) error sending message\n", pid);
    }
    else
    {
        // if bytes_sent > 0 but less than len, a partial send occurred
        printf("[PID %i] - ⚠️ (3/5) warning: only sent %zd of %zu bytes.\n", pid, bytes_sent, len);
    }

    recieve_response(sockfd);

    close(sockfd);
}
