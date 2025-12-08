/**
 * Summary: Implementation of HTTP protocol parsing, header manipulation, and file serving logic.
 *
 * @file http_parser.c
 * @authors: Anna Running Rabbit, Joseph Mills, Jordan Senko
 */
#include "c_http_parser.h"

#include <string.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>

// --- FUNCTIONS ---
/**
 * @brief Reads the remaining bytes of the HTTP body from the socket.
 *        This function is called after the headers and initial body chunk have been processed.
 *        It continues to read from the socket in BUFFER_SIZE chunks until the expected
 *        "remaining_bytes"have been received and written to the file.
 *
 * @param remaining_bytes # of bytes yet to be read
 * @param total_written Pointer to a counter tracking the total bytes written to the file
 * @param buffer A reusable buffer for storing incoming data chunks
 * @param sockfd The socket file descriptor to read from
 * @param outfile The file pointer where the data should be written
 * @param file_name The name of the file being saved (for logging purposes)
 */
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

/**
 * @brief Opens the output file and initiates the saving process.
 *        This function constructs the file path (in "client-side/client-reqs/"), opens the file
 *        for binary writing, writes any body data already received in the header buffer, and
 *        then calls read_remaining_body_bytes() to fetch the rest of the content.
 *
 * @param body_bytes # of bytes of the body that are in the header buffer
 * @param body_start Pointer to the start of the body data in the header buffer
 * @param content_len Total size of the file content (from Content-Length)
 * @param file_name Name of the file to save
 * @param sockfd The socket file descriptor
 * @return 0 on success, or -1 if the file could not be opened.
 */
int save_file(size_t body_bytes, char *body_start, int content_len, char *file_name, int sockfd)
{
    pid_t pid = getpid();

    FILE *outfile = NULL;
    char file_path[FILE_NAME_LEN]; // buffer for "client_reqs/filename"
    char buffer[BUFFER_SIZE];      // local buffer for receiving data

    snprintf(file_path, sizeof(file_path), "client-side/client-reqs/%s", file_name); // "client_reqs" cannot be a constant

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

/**
 * @brief Parses the Content-Length header value from the response buffer.
 * * @param buffer The null-terminated response header buffer.
 * @return The content length as an integer, or -1 if the header is missing or invalid.
 */
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

/**
 * @brief Parses the HTTP status code from the status line.
 * @param buffer The response header buffer (e.g., "HTTP/1.1 200 OK").
 * @return The integer status code, or 0 if parsing fails.
 */
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

/**
 * @brief Main function to handle receiving the server's response.
 *        This function reads the response headers, checks the status code for errors,
 *        parses metadata (like Content-Length and File-Name), and initiates file saving.
 * @param serverfd The socket file descriptor connected to the server.
 */
void receive_response(int serverfd)
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

/**
 * @brief Sends the HTTP request string to the server and awaits the response.
 * @param sockfd The socket file descriptor connected to the server.
 * @param request The null-terminated HTTP request string.
 */
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

    receive_response(sockfd);

    close(sockfd);
}