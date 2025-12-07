#include "http_parser.h"
#include "thread_pool.h"
#include "../lib/uthash.h"

#include <sys/stat.h>
int total_requests = 0;
pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;
// --- HTTP structures ---
typedef struct HTTPHeader
{
    char key[64]; // header name such as "Host" or "Content Type"
    char value[256];
    UT_hash_handle hh; // makes the header hashable
} HTTPHeader;          // full header example "Host: www.example.com";

typedef struct HTTPRequest
{
    char method[10];
    char path[1024];
    char version[10];
    HTTPHeader *headers; // pointer to head of the HTTPHeader hash table
} HTTPRequest;

void create_root_path(char *filepath, HTTPRequest *rq);
void delete_all_headers(HTTPHeader **headers);
void add_header_to_hash(HTTPHeader **headers, const char *key, const char *value);
void clean_request(char *buffer);
int parse_request(const char *buffer, HTTPRequest *rq);
int is_valid_method(char *method);
int is_valid_version(char *version);
int parse_request_line(const char *line, HTTPRequest *rq);
void parse_single_header(char *line, HTTPRequest *rq);
void handle_request(int clientfd, const char *buffer);
void send_error_response(const char *filepath, int clientfd, int status_code);
void serve_file(int clientfd, const char *filepath, off_t filesize);
const char *get_mime_type(const char *filepath);

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
        sprintf(filepath, "server-side/www%s", rq->path);
    }
}

/**
 * @brief Receives a message from the client socket.
 *
 * @param clientfd The client socket file descriptor.
 * @param buffer Pointer to the buffer where the received message will be stored.
 * @return The number of bytes received on success, -1 on failure.
 */
ssize_t receive_message(int clientfd, char *buffer)
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
 * @brief Cleans the entire HTTP Request, replacing '\r' with ' ',
 *        so that we can send HTTP requests with netcat. Also just makes
 *        handling of requests more flexible.
 * @param buffer The buffer that holds the request
 */
void clean_request(char *buffer)
{
    int i;

    for (i = 0; buffer[i] == '\0'; i++)
    {
        if (buffer[i] == '\r')
        {
            buffer[i] = ' ';
        }
    }
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

    clean_request(buffer_copy);

    line_token = strtok_r(buffer_copy, "\n", &saveptr_line); // get first line

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
    while ((line_token = strtok_r(NULL, "\n", &saveptr_line)) != NULL)
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
    return 200;        // Ok
}

int is_valid_method(char *method)
{
    const char *methods = {"GET"};
    int i;

    for (i = 0; i < (sizeof(*methods) / sizeof(methods[i])); i++)
    {
        if (strcmp(&(methods[i]), method) == 0)
        {
            return 1;
        }
    }
    return 0;
}

int is_valid_version(char *version)
{
    const char *versions = {"HTTP/1.1"}; // supported versions
    int i;

    for (i = 0; i < (sizeof(*versions) / sizeof(versions[i])); i++)
    {
        if (strcmp(&(versions[i]), version) == 0)
        {
            return 1;
        }
    }
    return 0;
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

    if (!is_valid_method(rq->method) || !is_valid_version(rq->version))
    {
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
void parse_single_header(char *line, HTTPRequest *rq)
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
    pthread_mutex_lock(&stats_mutex);   
    total_requests++; 
    pthread_mutex_unlock(&stats_mutex); 
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
    if (strcmp(rq.path, "/stats") == 0)
    {
        char response[1024];
        char body[512];
        pthread_mutex_lock(&stats_mutex);
        int current_total = total_requests;
        int current_queue = queue_count;
        pthread_mutex_unlock(&stats_mutex);
        // Build the HTML body with your variables
        // Note: Make sure 'total_requests' and 'queue_count' are accessible here (globals)
        sprintf(body, "<html>"
                      "<head><meta http-equiv=\"refresh\" content=\"1\"></head>"
                      "<body>"
                      "<h1>Server Status Dashboard</h1>"
                      "<p><strong>Active Worker Threads:</strong> %d</p>"
                      "<p><strong>Current Queue Size:</strong> %d</p>"
                      "<p><strong>Total Requests Served:</strong> %d</p>" 
                      "</body></html>", 
                      NUM_THREADS, current_queue, current_total);

        // Build the HTTP Header + Body
        sprintf(response, "HTTP/1.1 200 OK\r\n"
                          "Content-Type: text/html\r\n"
                          "Content-Length: %ld\r\n"
                          "Connection: close\r\n"
                          "\r\n"
                          "%s", strlen(body), body);
        send(clientfd, response, strlen(response), 0);
        delete_all_headers(&rq.headers); 
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
 * @brief Sends an error response to the client based on the status code.
 *
 * @param filepath The file path related to the error.
 * @param clientfd The client socket file descriptor.
 * @param status_code The HTTP status code indicating the type of error.
 */
void send_error_response(const char *filepath, int clientfd, int status_code)
{
    char response[256];

    // safely print error message to server console
    log_request(clientfd, "GET", (char *)filepath, status_code);

    // create response for client
    if (status_code == 400)
    {
        sprintf(response, "HTTP/1.1 400 Bad Request\r\n");
    }
    else if (status_code == 404)
    {
        sprintf(response, "HTTP/1.1 404 Not Found\r\n");
    }
    else
    {
        sprintf(response, "HTTP/1.1 500 Internal Server Error\r\n");
    }

    if (send(clientfd, response, strlen(response), 0) < 0)
    {
        printf(" ❌ Error sending error message.\n");
    }
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

    char *file_name = strrchr(filepath, '/');
    if (file_name)
    {
        file_name++; // skip the slash itself to get just file name
    }
    else
    {
        file_name = (char *)filepath; // fallback if no slashes found
    }

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
        perror("Failed to send header\n");
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
            printf("Failed to send file content\n");
            send_error_response(filepath, clientfd, 500);
            break;
        }
    }

    fclose(file);
    log_request(clientfd, "GET", (char *)filepath, 200); // safely prints here instead
    // printf("Served file: %s\n", filepath);
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
    return "application/octet-stream"; // fallback
}
