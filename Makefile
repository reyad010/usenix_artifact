# Compiler
CC := gcc

# Compiler Flags (using pkg-config for Jansson)
CFLAGS := -Wall -std=gnu99 -I./include -I/usr/include -O0 -pthread -D_GNU_SOURCE $(shell pkg-config --cflags jansson) -fPIC

# Linker Flags (using pkg-config for Jansson)
LDFLAGS := $(shell pkg-config --libs jansson) -lnuma -lm -ldl

# Directories
SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin
BENCHMARK_DIR := $(SRC_DIR)/benchmarks
BENCHMARK_OBJ_DIR := $(OBJ_DIR)/benchmarks

# Source files
SRCS := $(wildcard $(SRC_DIR)/*.c)
BENCHMARK_SRCS := $(wildcard $(BENCHMARK_DIR)/*.c)

# Object files
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
BENCHMARK_OBJS := $(patsubst $(BENCHMARK_DIR)/%.c,$(BENCHMARK_OBJ_DIR)/%.o,$(BENCHMARK_SRCS))
BENCHMARK_SO := $(patsubst $(BENCHMARK_DIR)/%.c,$(BIN_DIR)/%.so,$(BENCHMARK_SRCS))

# Additional Objects for Benchmark Libraries
SOCKET_MEMORY_OBJ := $(OBJ_DIR)/socket_memory.o
UTIL_OBJ := $(OBJ_DIR)/util.o
MSR_UTILS_OBJ := $(OBJ_DIR)/msr_utils.o

# Executable name
EXEC := $(BIN_DIR)/msr_program

# --- Rules ---

# Default target
all: clean $(EXEC) $(BENCHMARK_SO)

# Install dependencies
install-deps:
	@if ! pkg-config --exists jansson; then \
		echo "Jansson library not found. Attempting to install..."; \
		if command -v apt-get &> /dev/null; then \
			sudo apt-get update; \
			sudo apt-get install -y libjansson-dev; \
		elif command -v yum &> /dev/null; then \
			sudo yum install -y jansson-devel; \
		elif command -v dnf &> /dev/null; then \
			sudo dnf install -y jansson-devel; \
		elif command -v zypper &> /dev/null; then \
			sudo zypper install -y jansson-devel; \
		elif command -v pacman &> /dev/null; then \
			sudo pacman -S jansson --noconfirm; \
		else \
			echo "Error: Could not determine package manager to install libjansson-dev."; \
			echo "Please install libjansson-dev manually and then run 'make' again."; \
			exit 1; \
		fi; \
	else \
		echo "Jansson library is already installed."; \
	fi
	@sudo ldconfig # Update library cache after install

# Create object directories and compile .c files into .o files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

# Compile benchmarks into shared objects and link required dependencies
$(BENCHMARK_OBJ_DIR)/%.o: $(BENCHMARK_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Convert benchmark object files into shared libraries (.so) and link necessary objects
$(BIN_DIR)/%.so: $(BENCHMARK_OBJ_DIR)/%.o $(SOCKET_MEMORY_OBJ) $(UTIL_OBJ) $(MSR_UTILS_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) -shared -o $@ $< $(SOCKET_MEMORY_OBJ) $(UTIL_OBJ) $(MSR_UTILS_OBJ) $(LDFLAGS)

# Link object files to create the executable
$(EXEC): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(OBJS) -o $(EXEC) $(LDFLAGS)

# Clean build artifacts
clean:
	@rm -rf $(OBJ_DIR) $(BIN_DIR)

-include $(OBJS:.o=.d)

.PHONY: clean all install-deps
