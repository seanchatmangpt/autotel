# First Principles C Tutorials Makefile
# Validates core concepts for 7T Engine development

CC = cc
CFLAGS = -O3 -march=native -fPIC -Wall -Wextra -std=c99
INCLUDES = -I../compiler/src
LIBS = -lm

# Tutorial targets
TUTORIALS = 01_memory_management 02_performance_measurement 03_template_engine 04_query_engine 05_integration_patterns

# Default target
all: $(TUTORIALS)

# Memory Management Tutorial
01_memory_management: 01_memory_management.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

# Performance Measurement Tutorial
02_performance_measurement: 02_performance_measurement.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

# Template Engine Tutorial
03_template_engine: 03_template_engine.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

# Query Engine Tutorial
04_query_engine: 04_query_engine.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

# Integration Patterns Tutorial
05_integration_patterns: 05_integration_patterns.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

# Run all tutorials
tutorial: all
	@echo "Running First Principles C Tutorials"
	@echo "===================================="
	@echo ""
	
	@echo "1. Memory Management:"
	@./01_memory_management
	@echo ""
	
	@echo "2. Performance Measurement:"
	@./02_performance_measurement
	@echo ""
	
	@echo "3. Template Engine:"
	@./03_template_engine
	@echo ""
	
	@echo "4. Query Engine:"
	@./04_query_engine
	@echo ""
	
	@echo "5. Integration Patterns:"
	@./05_integration_patterns
	@echo ""
	
	@echo "First Principles Tutorials Complete!"

# Run individual tutorials
tutorial-memory: 01_memory_management
	@echo "Running Memory Management Tutorial..."
	@./01_memory_management

tutorial-performance: 02_performance_measurement
	@echo "Running Performance Measurement Tutorial..."
	@./02_performance_measurement

tutorial-template: 03_template_engine
	@echo "Running Template Engine Tutorial..."
	@./03_template_engine

tutorial-query: 04_query_engine
	@echo "Running Query Engine Tutorial..."
	@./04_query_engine

tutorial-integration: 05_integration_patterns
	@echo "Running Integration Patterns Tutorial..."
	@./05_integration_patterns

# Performance tests
perf-test: all
	@echo "Running Performance Tests..."
	@echo "============================"
	@echo ""
	
	@echo "Memory Management Performance:"
	@time ./01_memory_management > /dev/null 2>&1
	@echo ""
	
	@echo "Performance Measurement Performance:"
	@time ./02_performance_measurement > /dev/null 2>&1
	@echo ""
	
	@echo "Template Engine Performance:"
	@time ./03_template_engine > /dev/null 2>&1
	@echo ""
	
	@echo "Query Engine Performance:"
	@time ./04_query_engine > /dev/null 2>&1
	@echo ""
	
	@echo "Integration Patterns Performance:"
	@time ./05_integration_patterns > /dev/null 2>&1

# Clean up
clean:
	rm -f $(TUTORIALS)

# Install dependencies (if needed)
deps:
	@echo "Checking dependencies..."
	@which $(CC) > /dev/null || (echo "Error: $(CC) not found" && exit 1)
	@echo "Dependencies OK"

# Help
help:
	@echo "First Principles C Tutorials Makefile"
	@echo "===================================="
	@echo ""
	@echo "Targets:"
	@echo "  all              - Build all tutorial executables"
	@echo "  tutorial         - Run all tutorials"
	@echo "  tutorial-memory  - Run memory management tutorial only"
	@echo "  tutorial-performance - Run performance measurement tutorial only"
	@echo "  tutorial-template - Run template engine tutorial only"
	@echo "  tutorial-query   - Run query engine tutorial only"
	@echo "  tutorial-integration - Run integration patterns tutorial only"
	@echo "  perf-test        - Run performance tests"
	@echo "  clean            - Remove all generated files"
	@echo "  deps             - Check dependencies"
	@echo "  help             - Show this help"
	@echo ""
	@echo "Examples:"
	@echo "  make tutorial    # Run all tutorials"
	@echo "  make tutorial-memory # Run only memory tutorial"
	@echo "  make clean       # Clean up"

.PHONY: all tutorial tutorial-memory tutorial-performance tutorial-template tutorial-query tutorial-integration perf-test clean deps help 