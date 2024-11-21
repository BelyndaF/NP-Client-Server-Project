# Makefile for Online Bookstore/Library Server-Client Application

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -g

# SQLite3 library
LIBS = -lsqlite3

# Executable names
SERVER = server
CLIENT = client

# Source files
SERVER_SRC = server.c
CLIENT_SRC = client.c

# Build targets
all: $(SERVER) $(CLIENT)

# Build the server executable
$(SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $(SERVER) $(LIBS)

# Build the client executable
$(CLIENT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $(CLIENT)

# Clean the build
clean:
	rm -f $(SERVER) $(CLIENT)

# Run the server
run-server: $(SERVER)
	./$(SERVER)

# Run the client
run-client: $(CLIENT)
	./$(CLIENT)

# Phony targets (to avoid file conflicts)
.PHONY: all clean run-server run-client
