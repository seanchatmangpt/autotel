# ─────────────────────────────────────────────────────────────
# Makefile for CHATMAN-NANO-STACK (S7T Library)
# ─────────────────────────────────────────────────────────────

CC = gcc
CFLAGS = -std=c11 -O3 -march=native -Wall -Wextra -Werror
CFLAGS += -fno-exceptions -fno-rtti -fstrict-aliasing
CFLAGS += -DS7T_DEBUG  # Enable performance assertions

# Include paths
INCLUDES = -I./include

# Source files
DEMO_SRC = examples/s7t_demo.c

# Output
DEMO_BIN = s7t_demo

# Detect architecture for SIMD flags
UNAME_M := $(shell uname -m)
ifeq ($(UNAME_M),x86_64)
    CFLAGS += -msse2 -mavx2
else ifeq ($(UNAME_M),aarch64)
    CFLAGS += -march=armv8-a+simd
endif

# Default target
all: $(DEMO_BIN)

# Build demo
$(DEMO_BIN): $(DEMO_SRC)
	@echo "Building CHATMAN-NANO-STACK demo..."
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@
	@echo "Build complete: ./$@"

# Run demo
run: $(DEMO_BIN)
	@echo "Running CHATMAN-NANO-STACK demo..."
	@./$(DEMO_BIN)

# Performance profiling
perf: $(DEMO_BIN)
	@echo "Running with performance profiling..."
	perf record -e cycles,instructions,cache-misses ./$(DEMO_BIN)
	perf report

# Valgrind check (should show 0 allocations)
valgrind: $(DEMO_BIN)
	@echo "Running memory check (expecting 0 heap allocations)..."
	valgrind --leak-check=full --show-leak-kinds=all ./$(DEMO_BIN)

# Assembly output for inspection
asm: $(DEMO_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -S $< -o s7t_demo.s
	@echo "Assembly output: s7t_demo.s"

# Clean
clean:
	rm -f $(DEMO_BIN) s7t_demo.s perf.data perf.data.old

# Help
help:
	@echo "CHATMAN-NANO-STACK Makefile targets:"
	@echo "  make         - Build the demo"
	@echo "  make run     - Build and run the demo"
	@echo "  make perf    - Run with performance profiling"
	@echo "  make valgrind - Check for memory allocations (should be 0)"
	@echo "  make asm     - Generate assembly output"
	@echo "  make clean   - Remove build artifacts"

.PHONY: all run perf valgrind asm clean help