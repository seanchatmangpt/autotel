# TTL Parser Enhanced Makefile - Phase 2
CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -std=c11 -Iinclude
LDFLAGS = 
SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = bin
TESTDIR = tests
TOOLSDIR = tools

# Source files
CORE_SRCS = $(filter-out $(SRCDIR)/main.c $(SRCDIR)/main_query.c $(SRCDIR)/main_enhanced_demo.c, $(wildcard $(SRCDIR)/*.c))
CORE_OBJS = $(CORE_SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
DEPS = $(CORE_OBJS:.o=.d)

# Main targets
TARGET = $(BINDIR)/ttl-parser
QUERY_TARGET = $(BINDIR)/ttl-query
BENCHMARK_TARGET = $(BINDIR)/ttl-benchmark
TEST_TARGET = $(BINDIR)/test-runner

# Create directories
$(shell mkdir -p $(OBJDIR) $(BINDIR))

# Default target
all: $(TARGET) $(QUERY_TARGET) $(BENCHMARK_TARGET)

# Main parser executable
$(TARGET): $(CORE_OBJS) $(OBJDIR)/main.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Query interface executable
$(QUERY_TARGET): $(CORE_OBJS) $(OBJDIR)/main_query.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Benchmark tool
$(BENCHMARK_TARGET): $(CORE_OBJS) $(OBJDIR)/benchmark.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Test executable
test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(CORE_OBJS) $(wildcard $(TESTDIR)/*.c)
	$(CC) $(CFLAGS) -o $@ $(filter-out $(OBJDIR)/main.o $(OBJDIR)/main_query.o, $(CORE_OBJS)) $(TESTDIR)/*.c $(LDFLAGS)

# Object files from src
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -MMD -c $< -o $@

# Object files from tools
$(OBJDIR)/%.o: $(TOOLSDIR)/%.c
	$(CC) $(CFLAGS) -MMD -c $< -o $@

# Include dependencies
-include $(DEPS)

# Specific test targets
test-serializers: $(TARGET)
	@echo "Testing RDF serializers..."
	@./$(TARGET) --format ntriples examples/simple.ttl > /tmp/test.nt
	@./$(TARGET) --format jsonld examples/simple.ttl > /tmp/test.jsonld
	@./$(TARGET) --format rdfxml examples/simple.ttl > /tmp/test.rdf
	@echo "✓ All serializers working"

test-query: $(QUERY_TARGET)
	@echo "Testing query engine..."
	@./$(QUERY_TARGET) --query "?s rdf:type foaf:Person" examples/simple.ttl
	@./$(QUERY_TARGET) --query "?s foaf:name ?name" examples/simple.ttl
	@echo "✓ Query engine working"

test-cli: $(TARGET)
	@echo "Testing enhanced CLI..."
	@./$(TARGET) --stats examples/simple.ttl
	@./$(TARGET) --lint examples/simple.ttl
	@./$(TARGET) --format stats examples/simple.ttl
	@echo "✓ Enhanced CLI working"

benchmark: $(BENCHMARK_TARGET)
	@echo "Running performance benchmarks..."
	@./$(BENCHMARK_TARGET) examples/simple.ttl --suite

# Format conversion examples
convert-examples: $(TARGET)
	@echo "Creating format conversion examples..."
	@mkdir -p examples/formats
	@./$(TARGET) --format ntriples examples/simple.ttl > examples/formats/simple.nt
	@./$(TARGET) --format jsonld examples/simple.ttl > examples/formats/simple.jsonld
	@./$(TARGET) --format rdfxml examples/simple.ttl > examples/formats/simple.rdf
	@./$(TARGET) --format json examples/simple.ttl > examples/formats/simple.json
	@echo "✓ Format examples created in examples/formats/"

# Comprehensive test suite
test-all: test test-serializers test-query test-cli
	@echo "✓ All tests passed!"

# Performance test suite
test-performance: $(BENCHMARK_TARGET) $(TARGET)
	@echo "Running performance test suite..."
	@./$(BENCHMARK_TARGET) examples/simple.ttl --parse
	@./$(BENCHMARK_TARGET) examples/simple.ttl --query
	@./$(BENCHMARK_TARGET) examples/simple.ttl --compare

# Memory leak check
test-memory: $(TEST_TARGET)
	@echo "Running memory leak tests..."
	@valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TEST_TARGET)

# Static analysis
analyze:
	@echo "Running static analysis..."
	@clang --analyze $(CORE_SRCS)

# Install targets
install: $(TARGET) $(QUERY_TARGET)
	install -m 755 $(TARGET) /usr/local/bin/
	install -m 755 $(QUERY_TARGET) /usr/local/bin/

install-tools: $(BENCHMARK_TARGET)
	install -m 755 $(BENCHMARK_TARGET) /usr/local/bin/

# Documentation generation
docs:
	@echo "Generating documentation..."
	@doxygen docs/Doxyfile 2>/dev/null || echo "Doxygen not found, skipping docs"

# Clean targets
clean:
	rm -rf $(OBJDIR) $(BINDIR)

clean-examples:
	rm -rf examples/formats

clean-all: clean clean-examples

# Development helpers
format:
	clang-format -i $(SRCDIR)/*.c $(INCDIR)/*.h $(TESTDIR)/*.c $(TOOLSDIR)/*.c

lint:
	cpplint --filter=-whitespace/tabs $(SRCDIR)/*.c $(INCDIR)/*.h

# Package for distribution
package: clean all test-all convert-examples
	@echo "Creating distribution package..."
	@tar czf ttl-parser-v2.tar.gz \
		$(SRCDIR)/ $(INCDIR)/ $(TESTDIR)/ $(TOOLSDIR)/ \
		examples/ docs/ \
		Makefile README.md .gitignore
	@echo "✓ Package created: ttl-parser-v2.tar.gz"

# Quick demo
demo: $(TARGET) $(QUERY_TARGET) convert-examples
	@echo "\n=== TTL Parser Demo ==="
	@echo "1. Parsing simple TTL file:"
	@./$(TARGET) examples/simple.ttl
	@echo "\n2. Converting to N-Triples:"
	@./$(TARGET) --format ntriples examples/simple.ttl | head -5
	@echo "\n3. Querying for people:"
	@./$(QUERY_TARGET) --query "?s rdf:type foaf:Person" examples/simple.ttl
	@echo "\n4. Getting statistics:"
	@./$(TARGET) --stats examples/simple.ttl
	@echo "\n✓ Demo complete! Check examples/formats/ for output files."

.PHONY: all clean test install demo docs package format lint analyze
.PHONY: test-serializers test-query test-cli test-all test-performance test-memory
.PHONY: convert-examples benchmark clean-examples clean-all install-tools