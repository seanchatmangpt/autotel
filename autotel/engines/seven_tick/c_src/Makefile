CC = gcc
CFLAGS = -O3 -Wall -Wextra -march=native -std=c11 -D_GNU_SOURCE
LDFLAGS = -lm

# Source files
SOURCES = sparql7t.c shacl7t.c qop.c sparql_joins.c owl7t.c 7t_compliance.c 7t_compiler.c
HEADERS = sparql7t.h shacl7t.h qop.h sparql_joins.h owl7t.h shacl_advanced.h 7t_compliance.h 7t_compiler.h

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Benchmarks
BENCHMARKS = benchmark_simple benchmark_qop benchmark_ontology compliance_analyzer demo_tiers

# Default target
all: $(BENCHMARKS)

# Compile object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Build simple benchmark
benchmark_simple: benchmark_simple.o sparql7t.o shacl7t.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Build QOP benchmark
benchmark_qop: benchmark_qop.o qop.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Build ontology benchmark
benchmark_ontology: benchmark_ontology.o sparql7t.o shacl7t.o owl7t.o sparql_joins.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Run benchmarks
run: $(BENCHMARKS)
	@echo "Running Simple Benchmark..."
	./benchmark_simple	@echo ""
	@echo "Running QOP Benchmark..."
	./benchmark_qop

# Run only QOP benchmark
run-qop: benchmark_qop
	./benchmark_qop

# Clean build artifacts
clean:
	rm -f *.o $(BENCHMARKS)

# Rebuild everything
rebuild: clean all

.PHONY: all run run-qop clean rebuild# Build compliance analyzer
compliance_analyzer: compliance_analyzer.o 7t_compliance.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Build tier demo
demo_tiers: demo_tiers.o 7t_compiler.o 7t_compliance.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)