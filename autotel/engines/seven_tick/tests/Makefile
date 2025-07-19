# 7T Engine Unit Tests Makefile
# =============================

# Compiler and flags
CC = cc
CFLAGS = -O3 -march=native -fPIC -Wall -Wextra -std=c99
LDFLAGS = -lm

# Directories
SRC_DIR = ../c_src
RUNTIME_DIR = ../runtime/src
COMPILER_DIR = ../compiler/src
BENCHMARK_DIR = ../benchmarks

# Source files
RUNTIME_SOURCES = $(RUNTIME_DIR)/seven_t_runtime.c
TPOT_SOURCES = $(SRC_DIR)/7t_tpot.c
TELEMETRY_SOURCES = $(SRC_DIR)/telemetry7t.c
COMPILER_SOURCES = $(COMPILER_DIR)/cjinja.c $(COMPILER_DIR)/compiler.c $(COMPILER_DIR)/qop.c

# Test framework
TEST_FRAMEWORK_SOURCES = 7t_unit_test_framework.c
TEST_FRAMEWORK_HEADERS = 7t_unit_test_framework.h

# Test sources
TEST_SOURCES = test_seven_t_runtime.c test_7t_tpot.c test_telemetry7t.c
TEST_HEADERS = $(TEST_SOURCES:.c=.h)

# Object files
RUNTIME_OBJS = $(RUNTIME_SOURCES:.c=.o)
TPOT_OBJS = $(TPOT_SOURCES:.c=.o)
TELEMETRY_OBJS = $(TELEMETRY_SOURCES:.c=.o)
COMPILER_OBJS = $(COMPILER_SOURCES:.c=.o)
TEST_FRAMEWORK_OBJS = $(TEST_FRAMEWORK_SOURCES:.c=.o)
TEST_OBJS = $(TEST_SOURCES:.c=.o)

# Libraries
RUNTIME_LIB = lib7t_runtime.a
TPOT_LIB = lib7t_tpot.a
TELEMETRY_LIB = lib7t_telemetry.a
COMPILER_LIB = lib7t_compiler.a
TEST_FRAMEWORK_LIB = lib7t_test_framework.a

# Test executables
RUNTIME_TEST = test_seven_t_runtime
TPOT_TEST = test_7t_tpot
TELEMETRY_TEST = test_telemetry7t
ALL_TESTS = test_all

# Default target
.PHONY: all clean test test-runtime test-tpot test-telemetry test-all coverage

all: $(RUNTIME_TEST) $(TPOT_TEST) $(TELEMETRY_TEST) $(ALL_TESTS)

# ============================================================================
# LIBRARY BUILDING
# ============================================================================

# Runtime library
$(RUNTIME_LIB): $(RUNTIME_OBJS)
	ar rcs $@ $^

$(RUNTIME_DIR)/%.o: $(RUNTIME_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# TPOT library
$(TPOT_LIB): $(TPOT_OBJS)
	ar rcs $@ $^

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Telemetry library
$(TELEMETRY_LIB): $(TELEMETRY_OBJS)
	ar rcs $@ $^

# Compiler library
$(COMPILER_LIB): $(COMPILER_OBJS)
	ar rcs $@ $^

$(COMPILER_DIR)/%.o: $(COMPILER_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Test framework library
$(TEST_FRAMEWORK_LIB): $(TEST_FRAMEWORK_OBJS)
	ar rcs $@ $^

%.o: %.c $(TEST_FRAMEWORK_HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# ============================================================================
# TEST EXECUTABLES
# ============================================================================

# Runtime tests
$(RUNTIME_TEST): test_seven_t_runtime.o $(RUNTIME_LIB) $(TEST_FRAMEWORK_LIB)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# TPOT tests
$(TPOT_TEST): test_7t_tpot.o $(TPOT_LIB) $(RUNTIME_LIB) $(TEST_FRAMEWORK_LIB)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Telemetry tests
$(TELEMETRY_TEST): test_telemetry7t.o $(TELEMETRY_LIB) $(TEST_FRAMEWORK_LIB)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# All tests runner
$(ALL_TESTS): test_all.o $(RUNTIME_LIB) $(TPOT_LIB) $(TELEMETRY_LIB) $(TEST_FRAMEWORK_LIB)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# ============================================================================
# TEST RUNNING
# ============================================================================

test: test-all
	./$(ALL_TESTS)

test-runtime: $(RUNTIME_TEST)
	./$(RUNTIME_TEST)

test-tpot: $(TPOT_TEST)
	./$(TPOT_TEST)

test-telemetry: $(TELEMETRY_TEST)
	./$(TELEMETRY_TEST)

test-all: $(RUNTIME_TEST) $(TPOT_TEST) $(TELEMETRY_TEST)
	@echo "Running all unit tests..."
	@echo "=================================="
	@echo "Running Runtime Tests..."
	./$(RUNTIME_TEST)
	@echo ""
	@echo "Running TPOT Tests..."
	./$(TPOT_TEST)
	@echo ""
	@echo "Running Telemetry Tests..."
	./$(TELEMETRY_TEST)
	@echo ""
	@echo "All tests completed!"

# ============================================================================
# COVERAGE TESTING
# ============================================================================

# Coverage flags
COVERAGE_CFLAGS = $(CFLAGS) -fprofile-arcs -ftest-coverage
COVERAGE_LDFLAGS = $(LDFLAGS) -lgcov

# Coverage targets
coverage: CFLAGS = $(COVERAGE_CFLAGS)
coverage: LDFLAGS = $(COVERAGE_LDFLAGS)
coverage: clean test-all
	@echo "Generating coverage report..."
	lcov --capture --directory . --output-file coverage.info
	genhtml coverage.info --output-directory coverage_report
	@echo "Coverage report generated in coverage_report/"

# ============================================================================
# PERFORMANCE TESTING
# ============================================================================

perf-test: $(RUNTIME_TEST) $(TPOT_TEST) $(TELEMETRY_TEST)
	@echo "Running performance tests..."
	@echo "=================================="
	@echo "Runtime Performance Tests..."
	./$(RUNTIME_TEST) --performance
	@echo ""
	@echo "TPOT Performance Tests..."
	./$(TPOT_TEST) --performance
	@echo ""
	@echo "Telemetry Performance Tests..."
	./$(TELEMETRY_TEST) --performance

# ============================================================================
# MEMORY TESTING
# ============================================================================

mem-test: $(RUNTIME_TEST) $(TPOT_TEST) $(TELEMETRY_TEST)
	@echo "Running memory tests..."
	@echo "=================================="
	@echo "Runtime Memory Tests..."
	valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all ./$(RUNTIME_TEST)
	@echo ""
	@echo "TPOT Memory Tests..."
	valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all ./$(TPOT_TEST)
	@echo ""
	@echo "Telemetry Memory Tests..."
	valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all ./$(TELEMETRY_TEST)

# ============================================================================
# STRESS TESTING
# ============================================================================

stress-test: $(RUNTIME_TEST) $(TPOT_TEST) $(TELEMETRY_TEST)
	@echo "Running stress tests..."
	@echo "=================================="
	@echo "Runtime Stress Tests..."
	./$(RUNTIME_TEST) --stress
	@echo ""
	@echo "TPOT Stress Tests..."
	./$(TPOT_TEST) --stress
	@echo ""
	@echo "Telemetry Stress Tests..."
	./$(TELEMETRY_TEST) --stress

# ============================================================================
# CLEANUP
# ============================================================================

clean:
	rm -f *.o *.a *.gcno *.gcda *.gcov coverage.info
	rm -f $(RUNTIME_TEST) $(TPOT_TEST) $(TELEMETRY_TEST) $(ALL_TESTS)
	rm -f $(RUNTIME_OBJS) $(TPOT_OBJS) $(TELEMETRY_OBJS) $(COMPILER_OBJS)
	rm -rf coverage_report

distclean: clean
	rm -f *.log *.out *.err

# ============================================================================
# HELP
# ============================================================================

help:
	@echo "7T Engine Unit Tests Makefile"
	@echo "============================="
	@echo ""
	@echo "Available targets:"
	@echo "  all              - Build all test executables"
	@echo "  test             - Run all tests"
	@echo "  test-runtime     - Run runtime tests only"
	@echo "  test-tpot        - Run TPOT tests only"
	@echo "  test-telemetry   - Run telemetry tests only"
	@echo "  test-all         - Run all tests with detailed output"
	@echo "  coverage         - Generate coverage report"
	@echo "  perf-test        - Run performance tests"
	@echo "  mem-test         - Run memory tests with Valgrind"
	@echo "  stress-test      - Run stress tests"
	@echo "  clean            - Remove object files and executables"
	@echo "  distclean        - Remove all generated files"
	@echo "  help             - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make test-runtime    # Run only runtime tests"
	@echo "  make coverage        # Generate coverage report"
	@echo "  make mem-test        # Check for memory leaks"

# ============================================================================
# DEPENDENCIES
# ============================================================================

# Runtime test dependencies
test_seven_t_runtime.o: test_seven_t_runtime.c 7t_unit_test_framework.h $(RUNTIME_DIR)/seven_t_runtime.h

# TPOT test dependencies
test_7t_tpot.o: test_7t_tpot.c 7t_unit_test_framework.h $(SRC_DIR)/7t_tpot.h

# Telemetry test dependencies
test_telemetry7t.o: test_telemetry7t.c 7t_unit_test_framework.h $(SRC_DIR)/telemetry7t.h

# Test framework dependencies
7t_unit_test_framework.o: 7t_unit_test_framework.c 7t_unit_test_framework.h

# ============================================================================
# INSTALLATION
# ============================================================================

install: all
	@echo "Installing unit tests..."
	mkdir -p /usr/local/bin/7t-tests
	cp $(RUNTIME_TEST) $(TPOT_TEST) $(TELEMETRY_TEST) /usr/local/bin/7t-tests/
	@echo "Unit tests installed in /usr/local/bin/7t-tests/"

uninstall:
	@echo "Uninstalling unit tests..."
	rm -f /usr/local/bin/7t-tests/$(RUNTIME_TEST)
	rm -f /usr/local/bin/7t-tests/$(TPOT_TEST)
	rm -f /usr/local/bin/7t-tests/$(TELEMETRY_TEST)
	rmdir /usr/local/bin/7t-tests 2>/dev/null || true
	@echo "Unit tests uninstalled" 