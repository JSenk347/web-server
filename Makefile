CC = gcc

# 
CFLAGS = -Wall -g

# libraries to link (pthreads and lrt for semaphores)
LDFLAGS = -lpthread -lrt

# Default target: build both server and client
all: server client

# --- SERVER BUILD ---
server: server.o common.o
	$(CC) $(CFLAGS) server.o common.o -o server $(LDFLAGS)

server.o: server.c server.h common.h
	$(CC) $(CFLAGS) -c server.c

# --- CLIENT BUILD ---
client: client.o common.o
	$(CC) $(CFLAGS) client.o common.o -o client

client.o: client.c common.h
	$(CC) $(CFLAGS) -c client.c

# --- COMMON BUILD ---
common.o: common.c common.h
	$(CC) $(CFLAGS) -c common.c

# --- CLEAN UP ---
clean: 
	rm -f client client.o server server.o common.o