# CNS Unit Tests Makefile
# Builds and runs comprehensive unit tests for all CNS include files

CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2 -g
INCLUDES = -I../include -I../../include
LIBS = -lm

# Test executables
TEST_EXECUTABLES = test_cns_core test_cns_parser test_cns_dispatch test_cns_commands test_cns_benchmark test_cns_types test_cns_cli

# Default target
all: $(TEST_EXECUTABLES)

# Test targets
test_cns_core: test_cns_core.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

test_cns_parser: test_cns_parser.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

test_cns_dispatch: test_cns_dispatch.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

test_cns_commands: test_cns_commands.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

test_cns_benchmark: test_cns_benchmark.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

test_cns_types: test_cns_types.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

test_cns_cli: test_cns_cli.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

# Run all tests
test: all
	@echo "Running CNS Unit Tests"
	@echo "======================"
	@echo ""
	@for test in $(TEST_EXECUTABLES); do \
		echo "Running $$test..."; \
		./$$test; \
		echo ""; \
	done

# Run individual tests
test-core: test_cns_core
	./test_cns_core

test-parser: test_cns_parser
	./test_cns_parser

test-dispatch: test_cns_dispatch
	./test_cns_dispatch

test-commands: test_cns_commands
	./test_cns_commands

test-benchmark: test_cns_benchmark
	./test_cns_benchmark

test-types: test_cns_types
	./test_cns_types

test-cli: test_cns_cli
	./test_cns_cli

# Clean up
clean:
	rm -f $(TEST_EXECUTABLES)

# Install dependencies (if needed)
deps:
	@echo "Checking for required dependencies..."
	@which $(CC) > /dev/null || (echo "Error: $(CC) not found" && exit 1)
	@echo "Dependencies OK"

# Help
help:
	@echo "CNS Unit Tests Makefile"
	@echo "======================="
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build all test executables"
	@echo "  test         - Build and run all tests"
	@echo "  test-core    - Run CNS core tests"
	@echo "  test-parser  - Run CNS parser tests"
	@echo "  test-dispatch- Run CNS dispatch tests"
	@echo "  test-commands- Run CNS commands tests"
	@echo "  test-benchmark- Run CNS benchmark tests"
	@echo "  test-types   - Run CNS types tests"
	@echo "  test-cli     - Run CNS CLI tests"
	@echo "  clean        - Remove test executables"
	@echo "  deps         - Check dependencies"
	@echo "  help         - Show this help"
	@echo ""
	@echo "Examples:"
	@echo "  make test              # Run all tests"
	@echo "  make test-core         # Run only core tests"
	@echo "  make clean && make test # Clean build and test"

.PHONY: all test test-core test-parser test-dispatch test-commands test-benchmark test-types test-cli clean deps help 