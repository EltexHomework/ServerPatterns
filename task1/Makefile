CC := gcc
CFLAGS := -g -O2
LDFLAGS := -pthread 

# Directories
COMMON_SRC_DIR := common/src
CLIENT_SRC_DIR := client/src
CLIENT_HEADERS_DIR := client/headers
SERVER_SRC_DIR := server/src
SERVER_HEADERS_DIR := server/headers
REQUESTS_HEADERS_DIR := requests/headers
BIN_DIR := bin

# Include directories
INCLUDES := -I$(CLIENT_HEADERS_DIR) -I$(SERVER_HEADERS_DIR) -I$(REQUESTS_HEADERS_DIR)

# Source and object files for commons
COMMON_SOURCES := $(wildcard $(COMMON_SRC_DIR)/*.c)
COMMON_OBJECTS := $(patsubst $(COMMON_SRC_DIR)/%.c, $(BIN_DIR)/common_%.o, $(COMMON_SOURCES))

# Source and object files for client
CLIENT_SOURCES := $(wildcard $(CLIENT_SRC_DIR)/*.c)
CLIENT_OBJECTS := $(patsubst $(CLIENT_SRC_DIR)/%.c, $(BIN_DIR)/client_%.o, $(CLIENT_SOURCES))

# Source and object files for server
SERVER_SOURCES := $(wildcard $(SERVER_SRC_DIR)/*.c)
SERVER_OBJECTS := $(patsubst $(SERVER_SRC_DIR)/%.c, $(BIN_DIR)/server_%.o, $(SERVER_SOURCES))

# Targets
CLIENT_TARGET := $(BIN_DIR)/client
SERVER_TARGET := $(BIN_DIR)/server

all: $(BIN_DIR) $(SERVER_TARGET) $(CLIENT_TARGET)

# Create bin directory
$(BIN_DIR):
	@mkdir -p $@

# Link object files to create the client executable
$(CLIENT_TARGET): $(COMMON_OBJECTS) $(CLIENT_OBJECTS)
	$(CC) $(COMMON_OBJECTS) $(CLIENT_OBJECTS) $(LDFLAGS) -lncurses -o $@

# Link object files to create the server executable
$(SERVER_TARGET): $(COMMON_OBJECTS) $(SERVER_OBJECTS)
	$(CC) $(COMMON_OBJECTS) $(SERVER_OBJECTS) $(LDFLAGS) -o $@

# Compile common source files to object files
$(BIN_DIR)/common_%.o: $(COMMON_SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile client source files to object files
$(BIN_DIR)/client_%.o: $(CLIENT_SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile server source files to object files
$(BIN_DIR)/server_%.o: $(SERVER_SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Clean bin folder
clean:
	@rm -rf $(BIN_DIR)

.PHONY: all clean

