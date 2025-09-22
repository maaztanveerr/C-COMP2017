# TODO: make sure the rules for server client and markdown filled!
CC := gcc
CFLAGS := -Wall -Wextra -g 
CFLAGS += -Ilibs

# Source files 
SERVER_SRC := server.o commands.o markdown.o
CLIENT_SRC := client.o

# Output files
SERVER := server
CLIENT := client

# Default target (builds both server and client)
all: $(SERVER) $(CLIENT)

# link server
$(SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $@ $^ -pthread

# link client
$(CLIENT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $@ $^

# compile any .c to .o
%.o: source/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Dependencies
# server.o needs all four headers
server.o: libs/server.h libs/commands.h libs/markdown.h libs/document.h
commands.o: libs/commands.h libs/markdown.h libs/document.h
markdown.o: libs/markdown.h libs/document.h

# client.c doesn’t have its own .h, so just depend on the .c
client.o: source/client.c

# Clean target to remove compiled files
.PHONY: clean
clean:
	rm -f $(SERVER) $(CLIENT) *.o