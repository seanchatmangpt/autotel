# CNS 7-Tick Test Infrastructure Makefile
# Comprehensive build system for testing arena, interner, graph, and benchmarks

CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2 -g -march=native -mtune=native
DEBUG_FLAGS = -O0 -DDEBUG -fsanitize=address
PERF_FLAGS = -O3 -DNDEBUG -flto
INCLUDES = -I../include -I../../include

# Libraries
LIBS = -lm -lpthread

# Test executables
CORE_TESTS = test_cns_core test_cns_parser test_cns_dispatch test_cns_commands test_cns_benchmark test_cns_types test_cns_cli
NEW_TESTS = test_arena test_interner test_graph benchmark
ALL_TESTS = $(CORE_TESTS) $(NEW_TESTS)

# Default target
all: $(ALL_TESTS)

# 7-Tick Infrastructure Tests
test_arena: test_arena.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)
	@echo "Built arena allocator tests"

test_interner: test_interner.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)
	@echo "Built string interner tests"

test_graph: test_graph.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)
	@echo "Built graph representation tests"

benchmark: benchmark.c
	$(CC) $(PERF_FLAGS) $(INCLUDES) -o $@ $< $(LIBS)
	@echo "Built performance benchmark suite"

# Core CNS Tests (existing)
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

# Debug builds
debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(ALL_TESTS)
	@echo "Built all tests with debug flags"

# Performance builds
perf: CFLAGS = $(PERF_FLAGS) $(INCLUDES)
perf: $(NEW_TESTS)
	@echo "Built performance tests with optimizations"

# Run all tests
test: $(ALL_TESTS)
	@echo "Running CNS 7-Tick Test Suite"
	@echo "==============================="
	@echo ""
	@failed=0; \
	for test in $(ALL_TESTS); do \
		echo "Running $$test..."; \
		if ./$$test; then \
			echo "  ✓ PASSED"; \
		else \
			echo "  ✗ FAILED"; \
			failed=$$((failed + 1)); \
		fi; \
		echo ""; \
	done; \
	echo "=============================="; \
	if [ $$failed -eq 0 ]; then \
		echo "All tests PASSED"; \
		exit 0; \
	else \
		echo "$$failed test(s) FAILED"; \
		exit 1; \
	fi

# Run individual test categories
test-arena: test_arena
	@echo "Running Arena Tests..."
	./test_arena

test-interner: test_interner
	@echo "Running String Interner Tests..."
	./test_interner

test-graph: test_graph
	@echo "Running Graph Tests..."
	./test_graph

test-benchmark: benchmark
	@echo "Running Performance Benchmarks..."
	./benchmark

test-core: $(CORE_TESTS)
	@echo "Running Core CNS Tests..."
	@for test in $(CORE_TESTS); do \
		echo "Running $$test..."; \
		./$$test; \
		echo ""; \
	done

# Run with valgrind for memory checking
valgrind: $(NEW_TESTS)
	@echo "Running memory checks with Valgrind..."
	@for test in $(NEW_TESTS); do \
		echo "Checking $$test with Valgrind..."; \
		valgrind --leak-check=full --error-exitcode=1 ./$$test; \
		echo ""; \
	done

# Quick smoke test (fast subset)
smoke: test_arena test_interner
	@echo "Running smoke tests..."
	./test_arena > /dev/null && echo "Arena: PASS" || echo "Arena: FAIL"
	./test_interner > /dev/null && echo "Interner: PASS" || echo "Interner: FAIL"

# Performance validation
perf-validate: benchmark
	@echo "Running performance validation..."
	./benchmark | grep -E "(PASS|FAIL|compliance)"

# Continuous integration target
ci: clean all test
	@echo "CI pipeline completed successfully"

# Benchmark with detailed output
bench-detailed: benchmark
	@echo "Running detailed performance analysis..."
	./benchmark > benchmark_results.txt
	@cat benchmark_results.txt
	@echo "Results saved to benchmark_results.txt"

# Memory stress test
stress: test_arena test_graph
	@echo "Running memory stress tests..."
	./test_arena > /dev/null &
	./test_graph > /dev/null &
	wait
	@echo "Stress test completed"

# Coverage analysis (if gcov available)
coverage: CFLAGS += --coverage
coverage: $(NEW_TESTS)
	@echo "Running tests with coverage analysis..."
	@for test in $(NEW_TESTS); do \
		./$$test > /dev/null; \
	done
	@gcov *.c 2>/dev/null || echo "gcov not available"

# Static analysis (if available)
analyze:
	@echo "Running static analysis..."
	@which cppcheck > /dev/null && cppcheck --enable=all --std=c99 *.c || echo "cppcheck not available"
	@which clang-tidy > /dev/null && clang-tidy *.c -- $(INCLUDES) || echo "clang-tidy not available"

# Documentation generation
docs:
	@echo "Generating test documentation..."
	@echo "# CNS Test Infrastructure" > TEST_INFRASTRUCTURE.md
	@echo "" >> TEST_INFRASTRUCTURE.md
	@echo "## Test Files" >> TEST_INFRASTRUCTURE.md
	@for test in $(ALL_TESTS); do \
		echo "- $$test.c" >> TEST_INFRASTRUCTURE.md; \
	done
	@echo "" >> TEST_INFRASTRUCTURE.md
	@echo "## Usage" >> TEST_INFRASTRUCTURE.md
	@echo "\`\`\`bash" >> TEST_INFRASTRUCTURE.md
	@echo "make test        # Run all tests" >> TEST_INFRASTRUCTURE.md
	@echo "make test-arena  # Run arena tests" >> TEST_INFRASTRUCTURE.md
	@echo "make benchmark   # Run benchmarks" >> TEST_INFRASTRUCTURE.md
	@echo "\`\`\`" >> TEST_INFRASTRUCTURE.md
	@echo "Documentation generated: TEST_INFRASTRUCTURE.md"

# Clean build artifacts
clean:
	rm -f $(ALL_TESTS) *.o *.gcda *.gcno *.gcov benchmark_results.txt
	@echo "Cleaned build artifacts"

# Clean and rebuild
rebuild: clean all

# Install test framework (copy to system location)
install: $(ALL_TESTS)
	@echo "Installing CNS test framework..."
	mkdir -p /usr/local/bin/cns-tests
	cp $(ALL_TESTS) /usr/local/bin/cns-tests/
	@echo "Tests installed to /usr/local/bin/cns-tests/"

# Uninstall test framework
uninstall:
	rm -rf /usr/local/bin/cns-tests
	@echo "CNS test framework uninstalled"

# Dependencies check
deps:
	@echo "Checking dependencies..."
	@which $(CC) > /dev/null || (echo "Error: $(CC) not found" && exit 1)
	@echo "Compiler: OK"
	@echo "Dependencies: OK"

# Help target
help:
	@echo "CNS 7-Tick Test Infrastructure Makefile"
	@echo "========================================"
	@echo ""
	@echo "Targets:"
	@echo "  all               - Build all tests"
	@echo "  test              - Run all tests"
	@echo "  test-arena        - Run arena allocator tests"
	@echo "  test-interner     - Run string interner tests"
	@echo "  test-graph        - Run graph representation tests"
	@echo "  test-benchmark    - Run performance benchmarks"
	@echo "  test-core         - Run core CNS tests"
	@echo ""
	@echo "  debug             - Build with debug flags"
	@echo "  perf              - Build with performance optimizations"
	@echo "  valgrind          - Run with memory checking"
	@echo "  smoke             - Quick smoke test"
	@echo "  perf-validate     - Performance validation only"
	@echo "  ci                - Continuous integration pipeline"
	@echo ""
	@echo "  bench-detailed    - Detailed benchmark analysis"
	@echo "  stress            - Memory stress testing"
	@echo "  coverage          - Coverage analysis"
	@echo "  analyze           - Static analysis"
	@echo ""
	@echo "  clean             - Remove build artifacts"
	@echo "  rebuild           - Clean and rebuild"
	@echo "  docs              - Generate documentation"
	@echo "  deps              - Check dependencies"
	@echo "  help              - Show this help"
	@echo ""
	@echo "Examples:"
	@echo "  make test         # Run full test suite"
	@echo "  make smoke        # Quick validation"
	@echo "  make debug test   # Debug build and test"

# Special targets
.PHONY: all test debug perf test-arena test-interner test-graph test-benchmark test-core
.PHONY: valgrind smoke perf-validate ci bench-detailed stress coverage analyze
.PHONY: clean rebuild install uninstall deps help docs
.SECONDARY: # Keep intermediate files