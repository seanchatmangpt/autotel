# Makefile for 7T Demo Use Cases Benchmark Suite

# Compiler and flags
CC = gcc
CFLAGS = -O3 -march=native -mtune=native -ffast-math -funroll-loops -fomit-frame-pointer -Wall -Wextra
INCLUDES = -I../runtime/src
LIBS = -lm

# Directories
SRC_DIR = .
RUNTIME_DIR = ../runtime/src
BENCHMARK_DIR = ../benchmarks

# Source files
DEMO_SOURCES = demo_benchmark_framework.c demo_benchmark_suite.c
RUNTIME_SOURCES = $(RUNTIME_DIR)/seven_t_runtime.c
DEMO_USE_CASES = demo_use_case_1_iris.c demo_use_case_2_boston.c demo_use_case_3_digits.c demo_use_case_4_breast_cancer.c demo_use_case_5_wine.c

# Targets
DEMO_BENCHMARK = demo_benchmark_suite
DEMO_IRIS = demo_iris
DEMO_BOSTON = demo_boston
DEMO_DIGITS = demo_digits
DEMO_CANCER = demo_cancer
DEMO_WINE = demo_wine

# Default target
all: $(DEMO_BENCHMARK) $(DEMO_IRIS) $(DEMO_BOSTON) $(DEMO_DIGITS) $(DEMO_CANCER) $(DEMO_WINE)

# Main demo benchmark suite
$(DEMO_BENCHMARK): $(DEMO_SOURCES) $(RUNTIME_SOURCES)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)
	@echo "✅ Demo benchmark suite compiled successfully"

# Individual demo use case executables
$(DEMO_IRIS): demo_use_case_1_iris.c $(RUNTIME_SOURCES)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)
	@echo "✅ Iris classification demo compiled"

$(DEMO_BOSTON): demo_use_case_2_boston.c $(RUNTIME_SOURCES)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)
	@echo "✅ Boston housing regression demo compiled"

$(DEMO_DIGITS): demo_use_case_3_digits.c $(RUNTIME_SOURCES)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)
	@echo "✅ Digits classification demo compiled"

$(DEMO_CANCER): demo_use_case_4_breast_cancer.c $(RUNTIME_SOURCES)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)
	@echo "✅ Breast cancer classification demo compiled"

$(DEMO_WINE): demo_use_case_5_wine.c $(RUNTIME_SOURCES)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)
	@echo "✅ Wine quality classification demo compiled"

# Run targets
run-benchmark: $(DEMO_BENCHMARK)
	@echo "=== Running 7T Demo Use Cases Benchmark Suite ==="
	./$(DEMO_BENCHMARK)

run-iris: $(DEMO_IRIS)
	@echo "=== Running Iris Classification Demo ==="
	./$(DEMO_IRIS)

run-boston: $(DEMO_BOSTON)
	@echo "=== Running Boston Housing Regression Demo ==="
	./$(DEMO_BOSTON)

run-digits: $(DEMO_DIGITS)
	@echo "=== Running Digits Classification Demo ==="
	./$(DEMO_DIGITS)

run-cancer: $(DEMO_CANCER)
	@echo "=== Running Breast Cancer Classification Demo ==="
	./$(DEMO_CANCER)

run-wine: $(DEMO_WINE)
	@echo "=== Running Wine Quality Classification Demo ==="
	./$(DEMO_WINE)

run-all-demos: run-iris run-boston run-digits run-cancer run-wine
	@echo "✅ All demo use cases completed"

# Python runner targets
run-python-benchmark: $(DEMO_BENCHMARK)
	@echo "=== Running Python Benchmark Runner ==="
	python3 demo_benchmark_runner.py

run-python-benchmark-config: $(DEMO_BENCHMARK)
	@echo "=== Running Python Benchmark Runner with Config ==="
	python3 demo_benchmark_runner.py --config demo_benchmark_config.json

run-continuous-monitoring: $(DEMO_BENCHMARK)
	@echo "=== Running Continuous Monitoring (1 hour) ==="
	python3 demo_benchmark_runner.py --continuous 3600

# Performance optimization targets
optimize: CFLAGS += -DNDEBUG -fprofile-generate
optimize: clean all
	@echo "✅ Optimized build completed"

profile: CFLAGS += -pg
profile: clean all
	@echo "✅ Profiled build completed"

debug: CFLAGS = -g -O0 -Wall -Wextra
debug: clean all
	@echo "✅ Debug build completed"

# Clean targets
clean:
	rm -f $(DEMO_BENCHMARK) $(DEMO_IRIS) $(DEMO_BOSTON) $(DEMO_DIGITS) $(DEMO_CANCER) $(DEMO_WINE)
	rm -f *.o *.a *.so
	rm -f gmon.out
	@echo "✅ Clean completed"

clean-results:
	rm -rf demo_benchmark_results/
	@echo "✅ Results cleaned"

clean-all: clean clean-results
	@echo "✅ Complete clean completed"

# Test targets
TEST_OBJS = demo_benchmark_tests.o demo_benchmark_test_factories.o demo_benchmark_common.o demo_benchmark_framework.o demo_benchmark_suite.o
TEST_TARGET = demo_benchmark_tests

test: $(TEST_TARGET)
	@echo "🧪 Running 80/20 unit tests..."
	./$(TEST_TARGET)

test-factories: $(TEST_TARGET)
	@echo "🏭 Running factory-based tests..."
	./$(TEST_TARGET) factories

test-category: $(TEST_TARGET)
	@echo "🎯 Running specific test category..."
	@if [ -z "$(CATEGORY)" ]; then \
		echo "Usage: make test-category CATEGORY=<category>"; \
		echo "Available categories: matrix, array, dataset, algorithm, pipeline, benchmark, performance, memory, error, integration, factory, regression"; \
		exit 1; \
	fi
	./$(TEST_TARGET) $(CATEGORY)

$(TEST_TARGET): $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

demo_benchmark_tests.o: demo_benchmark_tests.c demo_benchmark_common.h demo_benchmark_framework.h demo_benchmark_test_factories.h
	$(CC) $(CFLAGS) -c $< -o $@

demo_benchmark_test_factories.o: demo_benchmark_test_factories.c demo_benchmark_test_factories.h demo_benchmark_common.h
	$(CC) $(CFLAGS) -c $< -o $@

# Documentation targets
create-config:
	@echo "Creating demo benchmark configuration file..."
	@cat > demo_benchmark_config.json << 'EOF'
{
    "target_cycles": 49,
    "target_ns": 1000,
    "target_fitness_min": 0.7,
    "target_fitness_max": 0.95,
    "iterations": {
        "iris_classification": 1000,
        "boston_regression": 1000,
        "digits_classification": 500,
        "breast_cancer_classification": 1000,
        "wine_quality_classification": 1000,
        "latency_benchmark": 10000,
        "throughput_benchmark": 5000,
        "fitness_quality_benchmark": 1000,
        "memory_efficiency_benchmark": 100,
        "integration_stress_benchmark": 2000
    },
    "output_formats": ["json", "csv", "html"],
    "generate_plots": true,
    "save_results": true,
    "continuous_monitoring": false,
    "monitoring_interval": 300,
    "monitoring_duration": 3600
}
EOF
	@echo "✅ Configuration file created: demo_benchmark_config.json"

# Help target
help:
	@echo "7T Demo Use Cases Benchmark Suite Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  all                    - Build all executables"
	@echo "  run-benchmark          - Run complete benchmark suite"
	@echo "  run-iris               - Run Iris classification demo"
	@echo "  run-boston             - Run Boston housing regression demo"
	@echo "  run-digits             - Run Digits classification demo"
	@echo "  run-cancer             - Run Breast cancer classification demo"
	@echo "  run-wine               - Run Wine quality classification demo"
	@echo "  run-all-demos          - Run all individual demos"
	@echo "  run-python-benchmark   - Run Python benchmark runner"
	@echo "  run-continuous-monitoring - Run continuous monitoring"
	@echo ""
	@echo "Build variants:"
	@echo "  optimize               - Build with profile-guided optimization"
	@echo "  profile                - Build with profiling enabled"
	@echo "  debug                  - Build with debug symbols"
	@echo ""
	@echo "Testing:"
	@echo "  test-compilation       - Test compilation of all targets"
	@echo "  test-execution         - Test execution of all demos"
	@echo "  test-benchmark         - Test benchmark suite execution"
	@echo ""
	@echo "Utility:"
	@echo "  clean                  - Remove executables and object files"
	@echo "  clean-results          - Remove benchmark results"
	@echo "  clean-all              - Complete cleanup"
	@echo "  create-config          - Create default configuration file"
	@echo "  help                   - Show this help message"

# Phony targets
.PHONY: all run-benchmark run-iris run-boston run-digits run-cancer run-wine run-all-demos
.PHONY: run-python-benchmark run-python-benchmark-config run-continuous-monitoring
.PHONY: optimize profile debug clean clean-results clean-all
.PHONY: test-compilation test-execution test-benchmark create-config help 