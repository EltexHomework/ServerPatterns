CC := gcc
CFLAGS := -g -O2

# Directories
COMMON_SRC_DIR := common/src
CLIENT_TCP_SRC_DIR := client_tcp/src
CLIENT_TCP_HEADERS_DIR := client_tcp/headers
CLIENT_UDP_SRC_DIR := client_udp/src
CLIENT_UDP_HEADERS_DIR := client_udp/headers
SERVER_SRC_DIR := server/src
SERVER_HEADERS_DIR := server/headers
BIN_DIR := bin
DSTANDARD := 0

# Include directories
INCLUDES := -I$(CLIENT_HEADERS_DIR) -I$(SERVER_HEADERS_DIR)

# Source and object files for commons
COMMON_SOURCES := $(wildcard $(COMMON_SRC_DIR)/*.c)
COMMON_OBJECTS := $(patsubst $(COMMON_SRC_DIR)/%.c, $(BIN_DIR)/common_%.o, $(COMMON_SOURCES))

# Source and object files for client
CLIENT_TCP_SOURCES := $(wildcard $(CLIENT_TCP_SRC_DIR)/*.c)
CLIENT_TCP_OBJECTS := $(patsubst $(CLIENT_TCP_SRC_DIR)/%.c, $(BIN_DIR)/client_tcp_%.o, $(CLIENT_TCP_SOURCES))

# Source and object files for client
CLIENT_UDP_SOURCES := $(wildcard $(CLIENT_UDP_SRC_DIR)/*.c)
CLIENT_UDP_OBJECTS := $(patsubst $(CLIENT_UDP_SRC_DIR)/%.c, $(BIN_DIR)/client_udp_%.o, $(CLIENT_UDP_SOURCES))

# Source and object files for server
SERVER_SOURCES := $(wildcard $(SERVER_SRC_DIR)/*.c)
SERVER_OBJECTS := $(patsubst $(SERVER_SRC_DIR)/%.c, $(BIN_DIR)/server_%.o, $(SERVER_SOURCES))

# Targets
CLIENT_TCP_TARGET := $(BIN_DIR)/client_tcp
CLIENT_UDP_TARGET := $(BIN_DIR)/client_udp
SERVER_TARGET := $(BIN_DIR)/server

all: $(BIN_DIR) $(SERVER_TARGET) $(CLIENT_TCP_TARGET) $(CLIENT_UDP_TARGET)

# Create bin directory
$(BIN_DIR):
	@mkdir -p $@

# Link object files to create the client executable
$(CLIENT_TCP_TARGET): $(COMMON_OBJECTS) $(CLIENT_TCP_OBJECTS)
	$(CC) $(COMMON_OBJECTS) $(CLIENT_TCP_OBJECTS) -o $@

# Link object files to create the client executable
$(CLIENT_UDP_TARGET): $(COMMON_OBJECTS) $(CLIENT_UDP_OBJECTS)
	$(CC) $(COMMON_OBJECTS) $(CLIENT_UDP_OBJECTS) -o $@

# Link object files to create the server executable
$(SERVER_TARGET): $(COMMON_OBJECTS) $(SERVER_OBJECTS)
	$(CC) $(COMMON_OBJECTS) $(SERVER_OBJECTS) -o $@

# Compile common source files to object files
$(BIN_DIR)/common_%.o: $(COMMON_SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile client source files to object files
$(BIN_DIR)/client_tcp_%.o: $(CLIENT_TCP_SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile client source files to object files
$(BIN_DIR)/client_udp_%.o: $(CLIENT_UDP_SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile server source files to object files with STANDARD defined
$(BIN_DIR)/server_%.o: $(SERVER_SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -DSTANDARD=$(DSTANDARD) -c $< -o $@

# Specific targets for select, poll, and epoll
select: DSTANDARD=0
select: $(SERVER_TARGET)

poll: DSTANDARD=1
poll: $(SERVER_TARGET)

epoll: DSTANDARD=2
epoll: $(SERVER_TARGET)

# Clean bin folder
clean:
	@rm -rf $(BIN_DIR)

.PHONY: all clean select poll epoll

