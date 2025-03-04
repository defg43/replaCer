# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -Werror -Wrestrict -O3 -fPIC -Iinclude -std=c23
LDFLAGS = -shared

# Library name and version
LIB_NAME = replaCer
LIB_VERSION = 1.0
LIB_TARGET = lib$(LIB_NAME).so.$(LIB_VERSION)

# Directories
SRC_DIR = src
INCLUDE_DIR = include

# Source and Object files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:.c=.o)

# Test files
TEST_SRC = test.c
TEST_OBJ = $(TEST_SRC:.c=.o)
TEST_EXEC = test_runner

# Submodule paths (Modify as needed)
SUBMODULES = str ion

# Default target: Build everything
all: submodules $(LIB_TARGET)

# Build shared library
$(LIB_TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

# Compile object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build and run tests
test: $(TEST_EXEC)
	@echo "Running tests..."
	./$(TEST_EXEC)
	@echo "All tests passed!"

# Compile test program (links against the shared library)
$(TEST_EXEC): $(TEST_OBJ) $(LIB_TARGET)
	$(CC) $(CFLAGS) -o $@ $(TEST_OBJ) -L. -l$(LIB_NAME) -Wl,-rpath,.

# Clean build files
clean:
	rm -f $(OBJS) $(LIB_TARGET) $(TEST_OBJ) $(TEST_EXEC)

# Handle Git submodules
submodules:
	@git submodule update --init --recursive
	@for dir in $(SUBMODULES); do \
		echo "Building submodule in $$dir..."; \
		$(MAKE) -C $$dir; \
	done

# PHONY targets
.PHONY: all clean test submodules
