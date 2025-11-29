## Current Capabilities
- Send HTTP request from client to server
- Server parses request and stores into HTTPRequest object, defined in server.h

## Current Tasks
- Implement servicing of an HTTP request on a SINGLE thread
    - Once completed, must be able to do for n-threads

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