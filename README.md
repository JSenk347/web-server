## Current Capabilities
- Send HTTP request from client to server
- Server parses request and stores into `HTTPRequest` object, defined in server.h
- Server services request, sending the HTTP response with codes, and headers
    - Server also send back requested file if it exists on the server
- Client reads the HTTP response and prints the output to the terminal
    - This includes the static file that was requested, currently only known to be working for **text/html**

## Recently Completed Tasks
- Server sends HTTP response with appropriate error codes (not tested thoroughly)
- Server sends requested file back to client
- Client recieves and prints content of requested file to the terminal
    - Currently only known to be working with text/html type
- `handle_request()` is being abstracted

## Current Tasks
- Implement servicing of an HTTP request on a SINGLE thread
    - Once completed, must be able to do for n-threads
    - Figure out how to demonstrate that the client has recieved what it requested, perhaps by setting up a simple React UI, or simply printing to the terminal

## Usage
- To clean:
    'make clean'
- To make/compile:
    'make'
- To test:
    open a second bash terminal
    run make
    on bash shell 1, run './server'
    on bash shell 2, run './client'
    check other terminal for message