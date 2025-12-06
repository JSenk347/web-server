CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lpthread -lrt

# Project Directories
SERVER_DIR = server-side
CLIENT_DIR = client-side

# Object Files Required for Linking
SERVER_OBJS = $(SERVER_DIR)/server.o $(SERVER_DIR)/http_parser.o $(SERVER_DIR)/thread_pool.o
CLIENT_OBJS = $(CLIENT_DIR)/client.o $(CLIENT_DIR)/c_http_parser.o

# Main Targets
all: server client

server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) $(SERVER_OBJS) -o server $(LDFLAGS)

client: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) $(CLIENT_OBJS) -o client

# The symbols used in the action below mean:
#   $< = The name of the prerequisite source file (e.g., server-side/server.c)
#   $@ = The name of the target object file (e.g., server-side/server.o)
$(SERVER_DIR)/%.o: $(SERVER_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(CLIENT_DIR)/%.o: $(CLIENT_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# --- CLEANUP ---
clean:
	rm -f server client $(SERVER_DIR)/*.o $(CLIENT_DIR)/*.o






