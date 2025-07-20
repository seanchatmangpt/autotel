# TTL Parser Test Suite Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -O0 -DDEBUG_MEMORY
INCLUDES = -I../src -I.
SRCDIR = ../src
TESTDIR = .

# Source files
PARSER_SRCS = $(SRCDIR)/lexer.c $(SRCDIR)/parser.c
TEST_SRCS = test_main.c test_lexer_new.c test_parser.c test_integration.c test_error_handling.c

# Object files
PARSER_OBJS = $(PARSER_SRCS:.c=.o)
TEST_OBJS = $(TEST_SRCS:.c=.o)

# Targets
TEST_RUNNER = test_runner
ALL_TARGETS = $(TEST_RUNNER)

.PHONY: all clean test run-tests help install-deps

all: $(ALL_TARGETS)

# Build test runner
$(TEST_RUNNER): $(PARSER_OBJS) $(TEST_OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

# Individual object files
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Run all tests
test: $(TEST_RUNNER)
	@echo "🚀 Running TTL Parser Test Suite"
	@echo "================================="
	./$(TEST_RUNNER)

# Run specific test suites
test-lexer: $(TEST_RUNNER)
	@echo "🔍 Running Lexer Tests"
	./$(TEST_RUNNER) --suite lexer

test-parser: $(TEST_RUNNER)
	@echo "🔍 Running Parser Tests"
	./$(TEST_RUNNER) --suite parser

test-integration: $(TEST_RUNNER)
	@echo "🔍 Running Integration Tests"
	./$(TEST_RUNNER) --suite integration

test-error: $(TEST_RUNNER)
	@echo "🔍 Running Error Handling Tests"
	./$(TEST_RUNNER) --suite error

# Run tests with memory debugging
test-memory: $(TEST_RUNNER)
	@echo "🧠 Running Tests with Memory Debugging"
	./$(TEST_RUNNER) --memory

# Run tests verbosely
test-verbose: $(TEST_RUNNER)
	@echo "📝 Running Tests with Verbose Output"
	./$(TEST_RUNNER) --verbose

# Quick tests (skip slow ones)
test-quick: $(TEST_RUNNER)
	@echo "⚡ Running Quick Tests"
	./$(TEST_RUNNER) --quick

# Run tests with valgrind (if available)
test-valgrind: $(TEST_RUNNER)
	@echo "🔍 Running Tests with Valgrind"
	@if command -v valgrind >/dev/null 2>&1; then \
		valgrind --leak-check=full --track-origins=yes ./$(TEST_RUNNER); \
	else \
		echo "Valgrind not found, running tests normally"; \
		./$(TEST_RUNNER); \
	fi

# Run tests with AddressSanitizer
test-asan: CFLAGS += -fsanitize=address -fno-omit-frame-pointer
test-asan: clean $(TEST_RUNNER)
	@echo "🛡️  Running Tests with AddressSanitizer"
	./$(TEST_RUNNER)

# Performance benchmarking
test-perf: $(TEST_RUNNER)
	@echo "📊 Running Performance Tests"
	./$(TEST_RUNNER) --performance

# Generate test coverage report (if gcov available)
test-coverage: CFLAGS += --coverage
test-coverage: clean $(TEST_RUNNER)
	@echo "📈 Generating Test Coverage Report"
	./$(TEST_RUNNER)
	@if command -v gcov >/dev/null 2>&1; then \
		gcov $(PARSER_SRCS) $(TEST_SRCS); \
		echo "Coverage files generated: *.gcov"; \
	else \
		echo "gcov not found, coverage analysis not available"; \
	fi

# Install development dependencies
install-deps:
	@echo "📦 Installing Development Dependencies"
	@if command -v apt-get >/dev/null 2>&1; then \
		sudo apt-get update && sudo apt-get install -y valgrind build-essential; \
	elif command -v brew >/dev/null 2>&1; then \
		brew install valgrind; \
	elif command -v yum >/dev/null 2>&1; then \
		sudo yum install -y valgrind gcc; \
	else \
		echo "Package manager not detected. Please install valgrind and build tools manually."; \
	fi

# Clean build artifacts
clean:
	@echo "🧹 Cleaning build artifacts"
	rm -f $(ALL_TARGETS)
	rm -f $(PARSER_OBJS) $(TEST_OBJS)
	rm -f *.gcov *.gcda *.gcno
	rm -f core.*

# Create test report
test-report: $(TEST_RUNNER)
	@echo "📋 Generating Test Report"
	@echo "=========================" > test_report.txt
	@echo "TTL Parser Test Report" >> test_report.txt
	@echo "Generated: $$(date)" >> test_report.txt
	@echo "=========================" >> test_report.txt
	@echo "" >> test_report.txt
	./$(TEST_RUNNER) 2>&1 | tee -a test_report.txt
	@echo "" >> test_report.txt
	@echo "Report saved to: test_report.txt"

# Watch for file changes and run tests (requires inotify-tools on Linux)
test-watch:
	@echo "👀 Watching for file changes..."
	@if command -v inotifywait >/dev/null 2>&1; then \
		while inotifywait -e modify $(SRCDIR)/*.c $(SRCDIR)/*.h $(TESTDIR)/*.c $(TESTDIR)/*.h; do \
			clear; \
			echo "🔄 Files changed, running tests..."; \
			make test || true; \
		done; \
	elif command -v fswatch >/dev/null 2>&1; then \
		fswatch -o $(SRCDIR) $(TESTDIR) | while read num; do \
			clear; \
			echo "🔄 Files changed, running tests..."; \
			make test || true; \
		done; \
	else \
		echo "File watching tools not found. Install inotify-tools (Linux) or fswatch (macOS)"; \
	fi

# Debug build
debug: CFLAGS += -DDEBUG -g3
debug: $(TEST_RUNNER)
	@echo "🐛 Debug build complete"

# Release build (optimized)
release: CFLAGS = -Wall -Wextra -std=c99 -O3 -DNDEBUG
release: clean $(TEST_RUNNER)
	@echo "🚀 Release build complete"

# Static analysis (if available)
analyze:
	@echo "🔍 Running Static Analysis"
	@if command -v cppcheck >/dev/null 2>&1; then \
		cppcheck --enable=all --error-exitcode=1 $(SRCDIR)/*.c $(TESTDIR)/*.c; \
	elif command -v clang-analyzer >/dev/null 2>&1; then \
		scan-build make clean $(TEST_RUNNER); \
	else \
		echo "Static analysis tools not found. Install cppcheck or clang-analyzer"; \
	fi

# Help
help:
	@echo "TTL Parser Test Suite - Available Commands:"
	@echo "==========================================="
	@echo ""
	@echo "Building:"
	@echo "  make all           - Build all targets"
	@echo "  make debug         - Build with debug flags"
	@echo "  make release       - Build optimized release"
	@echo "  make clean         - Clean build artifacts"
	@echo ""
	@echo "Testing:"
	@echo "  make test          - Run all tests"
	@echo "  make test-lexer    - Run lexer tests only"
	@echo "  make test-parser   - Run parser tests only"
	@echo "  make test-integration - Run integration tests"
	@echo "  make test-error    - Run error handling tests"
	@echo "  make test-quick    - Run quick tests only"
	@echo "  make test-verbose  - Run tests with verbose output"
	@echo "  make test-memory   - Run tests with memory debugging"
	@echo ""
	@echo "Quality Assurance:"
	@echo "  make test-valgrind - Run tests with Valgrind"
	@echo "  make test-asan     - Run tests with AddressSanitizer"
	@echo "  make test-coverage - Generate test coverage report"
	@echo "  make analyze       - Run static analysis"
	@echo ""
	@echo "Utilities:"
	@echo "  make test-report   - Generate test report file"
	@echo "  make test-watch    - Watch for changes and run tests"
	@echo "  make install-deps  - Install development dependencies"
	@echo "  make help          - Show this help"

# Dependencies
$(SRCDIR)/lexer.o: $(SRCDIR)/lexer.c $(SRCDIR)/lexer.h
$(SRCDIR)/parser.o: $(SRCDIR)/parser.c $(SRCDIR)/parser.h $(SRCDIR)/lexer.h
test_main.o: test_main.c test_utils.h
test_lexer_new.o: test_lexer_new.c test_utils.h $(SRCDIR)/lexer.h
test_parser.o: test_parser.c test_utils.h $(SRCDIR)/parser.h $(SRCDIR)/lexer.h
test_integration.o: test_integration.c test_utils.h $(SRCDIR)/parser.h $(SRCDIR)/lexer.h
test_error_handling.o: test_error_handling.c test_utils.h $(SRCDIR)/parser.h $(SRCDIR)/lexer.h