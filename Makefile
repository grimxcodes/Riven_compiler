# Riven Native Compiler v1.0 - Professional Build System
# Target: Termux (Android), Linux, macOS

# Compiler and Flags
CC = clang
CFLAGS = -Wall -Wextra -std=c11 -Iinclude
LIBS = -lpthread

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
SYS_DIR = system

# Source Files for the Compiler (rvn)
# Note: runtime.c is not compiled into rvn; it is used during 'forge' to link with user code.
COMPILER_SRCS = $(SRC_DIR)/main.c \
                $(SRC_DIR)/lexer.c \
                $(SRC_DIR)/parser.c \
                $(SRC_DIR)/ast.c \
                $(SRC_DIR)/environment.c \
                $(SRC_DIR)/codegen.c

# Output Binary
TARGET = rvn

# Build Rules
all: $(TARGET)
	@echo "Riven Compiler (rvn) built successfully."
	@echo "Usage: ./rvn forge <file.rv>"

$(TARGET): $(COMPILER_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COMPILER_SRCS) -o $(TARGET) $(LIBS)

# Clean Build Artifacts
clean:
	@echo "Cleaning up..."
	rm -f $(TARGET)
	rm -f riven_native.c
	rm -f riven_output
	rm -rf $(BUILD_DIR)
	@echo "Clean complete."

# Install (Optional)
install: $(TARGET)
	@echo "Installing rvn to /usr/local/bin..."
	@cp $(TARGET) /usr/local/bin/rvn
	@chmod +x /usr/local/bin/rvn
	@echo "Install complete."

# Self-Test: Compile the hello world example
test: $(TARGET)
	@echo "Running Riven Self-Test..."
	./$(TARGET) run examples/hello.rv

.PHONY: all clean install test
