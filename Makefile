# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread
LDFLAGS = -lrt -lm

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BIN_DIR = bin

# Binaries (archivos con main)
BIN = $(BIN_DIR)/master $(BIN_DIR)/player $(BIN_DIR)/view

# Default target
all: $(BIN)

# Pattern rule for building binaries (compila cada archivo .c del directorio src que corresponda al binario)
$(BIN_DIR)/%: $(SRC_DIR)/%.c $(wildcard $(SRC_DIR)/shm.c)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -o $@ $^ $(LDFLAGS)

# Clean target
clean:
	rm -f $(BIN)

# Format target
format:
	clang-format -style=file --sort-includes --Werror -i $(SRC_DIR)/*.c $(INCLUDE_DIR)/*.h

# Help target
help:
	@echo "Available targets:"
	@echo "  all     - Build all binaries"
	@echo "  clean   - Remove all binaries"
	@echo "  format  - Format source code"
	@echo "  help    - Display this help message"

.PHONY: all clean format help
