# Multi-Threaded HTTP Web Server
**Authors**: Anna Running Rabbit, Joseph Mills, Jordan Senko
**Course**: COMP 3659 - Operating Systems
**Date**: November 2025 - December 2025

## Project Overview
This project implements a lightweight, multi-threaded HTTP web server in C. It is designed to handle concurrent client connections efficiently using a thread pool architecture. The server parses HTTP GET requests, serves static files (HTML, CSS, JS, images), and returns appropriate HTTP status codes.

The core objective is to demonstrate key operating system concepts, including:
- **Multi-threading**: Utilizing POSIX threads (pthreads) for concurrent request processing.
- **Synchronization**: employing mutexes and condition variables to manage a thread-safe producer-consumer request queue.
- **Socket Programming**: Using the low-level socket API for TCP/IP networking.
- **File I/O**: Efficiently reading and serving static resources.

## Features
- **Concurrent Handling**: Uses a fixed-size thread pool to handle multiple client connections simultaneously without blocking the main listener thread.
- **HTTP Parsing**: robustly parses HTTP GET requests to extract the method, path, and version.
- **Static File Serving**: Supports serving a variety of file types (HTML, CSS, JavaScript, images) with correct MIME types.
- **Error Handling**: Returns standard HTTP status codes:
    - `200 OK`
    - `400 Bad Request` (for malformed requests)
    - `404 Not Found` (for missing files)
    - `500 Internal Server Error` (for server-side issues)
- **Security**: Basic path traversal protection (blocks .. in paths).
- **Logging**: Thread-safe logging of requests to the console.

## Architecture
The server follows a **Producer-Consumer** model:
1. **Main Thread (Producer)**: Listens on the specified port. When a client connects, it accepts the connection and enqueues the client socket descriptor into a thread-safe queue. 
2. **Worker Threads (Consumers)**: A pool of worker threads waits for connections. When a socket is available, a worker dequeues it, reads the request, processes it, sends the response, and closes the connection.
Synchronization is managed using:
- `pthread_mutex_t` to protect the shared request queue and logging output.
- `pthread_cond_t` to signal worker threads when a new connection is available.

## Build Instructions
To compile the project, run the following command in the root directory:
```text
make
```
This will generate 2 executables: `server` and `client`.
To clean up the executables:
```text
make clean
```

## Usage
### Running the Server
Start the server on the default port (6767 hehe):
```text
./server
```
You should see the output indicatinf the thread pool initialization that the server is listening.
### Running the Client
The procided client is a testing utility that sends a specific HTTP request to the server.
```text
./client
```

### Modifying the Client Request
To change the file being requested, modify the `message` string in `client-side/client.c`:
```C
const char message[] =
    // Change the path below to test different scenarios:
    // "GET /A6.pdf HTTP/1.1\r\n"       // Valid Request
    // "GET /../server.c HTTP/1.1\r\n"  // Security Test (Path Traversal)
    // "GET /missing.txt HTTP/1.1\r\n"  // 404 Test
    "GET /HTTPSlides.png HTTP/1.1\r\n"      // Current Request
    "Host: 127.0.0.1:6767\r\n"
    "Connection: close\r\n"
    "\r\n";
```
After modifying `client.c`, rebuild the project:
```text
make client
```

### Requests in your Browser
You can also request files from within your browser if `server` is running.
- Example URL: `http://127.0.0.1:6767/PP2_Concept_Memo.pdf`

## Directory Structure
- `server-side/`: Contains server source code (`server.c`, `thread_pool.c`, `http_parser.c`) and the web root (`www/`)
- `client-side/`: Contains the test client source code.
- `lib/`: Shared libraries (e.g., `uthash.h`).