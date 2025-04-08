# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread
LDFLAGS = -lrt -lm

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BIN_DIR = bin

# Source files and binaries
SRC = $(wildcard $(SRC_DIR)/*.c)
BIN = $(BIN_DIR)/master $(BIN_DIR)/player $(BIN_DIR)/view

# Default target
all: $(BIN)

# Pattern rule for building binaries
$(BIN_DIR)/%: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -o $@ $< $(LDFLAGS)

# Clean target
clean:
	rm -f $(BIN)

# Format target
format:
	clang-format -style=file --sort-includes --Werror -i $(SRC_DIR)/*.c $(INCLUDE_DIR)/*.h

# Run target
run: all
	$(BIN_DIR)/master

# Help target
help:
	@echo "Available targets:"
	@echo "  all     - Build all binaries"
	@echo "  clean   - Remove all binaries"
	@echo "  format  - Format source code"
	@echo "  run     - Build and run the master binary"
	@echo "  help    - Display this help message"

# Phony targets
.PHONY: all clean format run help