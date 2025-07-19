# CNS Makefile - CHATMAN NANO-STACK
# 7-tick performance-guaranteed CLI with OpenTelemetry

CC = clang
CXX = clang++
CFLAGS = -O3 -march=native -flto -ffast-math -Wall -Wextra -std=c11 -DS7T_DEBUG=1
CXXFLAGS = -O3 -march=native -flto -ffast-math -Wall -Wextra -std=c++17
LDFLAGS = -flto -lm -lpthread
INCLUDES = -Iinclude -I../include -I../../c_src -I../../benchmarks -I../../tests

# Enable AVX2 on supported platforms
ifeq ($(shell uname -m),x86_64)
    CFLAGS += -mavx2 -mfma
    CXXFLAGS += -mavx2 -mfma
endif

# OpenTelemetry configuration
OTEL_ENABLED ?= 1
ifeq ($(OTEL_ENABLED),1)
    CFLAGS += -DWITH_OPENTELEMETRY
    CXXFLAGS += -DWITH_OPENTELEMETRY
    OTEL_LIBS = -lopentelemetry_trace -lopentelemetry_common -lopentelemetry_exporter_ostream_span
    LDFLAGS += $(OTEL_LIBS) -lstdc++
endif

# Source files
CORE_SRCS = src/core/cli.c src/cns_parser.c src/performance_optimizations.c
DOMAIN_SRCS = src/domains/sparql.c src/domains/shacl.c src/domains/cjinja.c src/domains/telemetry.c src/domains/bench.c src/domains/build.c src/domains/dashboard.c src/domains/deploy.c src/domains/docs.c src/domains/gate.c src/domains/parse.c src/domains/profile.c src/domains/release.c src/domains/sigma.c src/domains/ml.c src/domains/benchmark.c
ENGINE_SRCS = src/engines/sparql.c src/engines/shacl.c src/engines/cjinja.c src/engines/telemetry.c
# SQL_SRCS = src/domains/sql/sql_domain.c src/domains/sql/sql_parser.c src/domains/sql/sql_execute.c
# COMMAND_SRCS = src/cmd_spin.c src/cmd_think.c src/cmd_reflect.c src/cmd_learn.c src/cmd_adapt.c \
#                src/cmd_benchmark.c src/cmd_ml.c src/cmd_pm.c src/cmd_trace.c src/cmd_stubs.c
GATEKEEPER_SRCS = src/gatekeeper.c
WEAVER_SRCS = codegen/weaver_main.c
MAIN_SRC = src/main.c
MAIN_OTEL_SRC = src/cns_main.c

# Object files
CORE_OBJS = $(CORE_SRCS:.c=.o)
DOMAIN_OBJS = $(DOMAIN_SRCS:.c=.o)
ENGINE_OBJS = $(ENGINE_SRCS:.c=.o)
PERF_OBJS = $(PERF_SRCS:.c=.o)
# SQL_OBJS = $(SQL_SRCS:.c=.o)
# COMMAND_OBJS = $(COMMAND_SRCS:.c=.o)
GATEKEEPER_OBJS = $(GATEKEEPER_SRCS:.c=.o)
WEAVER_OBJS = $(WEAVER_SRCS:.c=.o)
MAIN_OBJ = $(MAIN_SRC:.c=.o)
MAIN_OTEL_OBJ = $(MAIN_OTEL_SRC:.c=.o)

# Choose main file based on OTel enablement
ifeq ($(OTEL_ENABLED),1)
    ifneq ($(wildcard $(MAIN_OTEL_SRC)),)
        ALL_OBJS = $(CORE_OBJS) $(DOMAIN_OBJS) $(ENGINE_OBJS) $(PERF_OBJS) $(MAIN_OTEL_OBJ)
    else
        ALL_OBJS = $(CORE_OBJS) $(DOMAIN_OBJS) $(ENGINE_OBJS) $(PERF_OBJS) $(MAIN_OBJ)
    endif
else
    ALL_OBJS = $(CORE_OBJS) $(DOMAIN_OBJS) $(ENGINE_OBJS) $(PERF_OBJS) $(MAIN_OBJ)
endif

# Binary names
TARGET = cns
GATEKEEPER_TARGET = gatekeeper
WEAVER_TARGET = cns_weaver

# Default target
all: stubs $(TARGET) $(GATEKEEPER_TARGET)
# Only build weaver if jansson is available
ifeq ($(shell pkg-config --exists jansson && echo yes),yes)
all: $(WEAVER_TARGET)
endif

# Create necessary directories and stub files
stubs:
	@mkdir -p src/domains/sql src/core
	@# Create stub domain files if they don't exist
	@for domain in build test bench gate parse dashboard docs release profile deploy sigma; do \
		if [ ! -f src/domains/$${domain}_domain.c ]; then \
			echo "Creating stub for $${domain}_domain..."; \
			echo '#include "cns/cli.h"' > src/domains/$${domain}_domain.c; \
			echo "CNSDomain cns_$${domain}_domain = {" >> src/domains/$${domain}_domain.c; \
			echo "    .name = \"$$domain\"," >> src/domains/$${domain}_domain.c; \
			echo "    .description = \"$$domain domain\"," >> src/domains/$${domain}_domain.c; \
			echo "    .commands = NULL," >> src/domains/$${domain}_domain.c; \
			echo "    .command_count = 0" >> src/domains/$${domain}_domain.c; \
			echo "};" >> src/domains/$${domain}_domain.c; \
		fi \
	done

# Build the main binary
$(TARGET): $(ALL_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^
ifeq ($(OTEL_ENABLED),1)
	@echo "✓ Built $(TARGET) with 7T-SQL and OpenTelemetry support"
else
	@echo "✓ Built $(TARGET) with 7T-SQL support (no OpenTelemetry)"
endif

# Build Gatekeeper
$(GATEKEEPER_TARGET): $(GATEKEEPER_OBJS) $(CORE_OBJS) $(ENGINE_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "✓ Built $(GATEKEEPER_TARGET) - CNS quality gate"

# Build Weaver (optional - requires jansson)
$(WEAVER_TARGET): $(WEAVER_OBJS)
ifeq ($(shell pkg-config --exists jansson && echo yes),yes)
	$(CC) $(LDFLAGS) -ljansson -lcjinja -o $@ $^
	@echo "✓ Built $(WEAVER_TARGET) - OpenTelemetry code generator"
else
	@echo "⚠ Skipping $(WEAVER_TARGET) - jansson library not found"
	@touch $(WEAVER_TARGET)
endif

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# Clean build artifacts
clean:
	rm -f $(CORE_OBJS) $(DOMAIN_OBJS) $(ENGINE_OBJS) $(SQL_OBJS) $(COMMAND_OBJS) $(GATEKEEPER_OBJS) $(WEAVER_OBJS) $(MAIN_OBJ) $(MAIN_OTEL_OBJ) $(TARGET) $(GATEKEEPER_TARGET) $(WEAVER_TARGET)
	rm -f src/domains/sql/*.o src/engines/*.o
	rm -f src/cns_otel.h src/cns_otel_inject.c
	@echo "✓ Cleaned build artifacts"

# Install binary
install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/
	@echo "✓ Installed to /usr/local/bin/$(TARGET)"

# Uninstall binary
uninstall:
	rm -f /usr/local/bin/$(TARGET)
	@echo "✓ Uninstalled from /usr/local/bin/$(TARGET)"

# Run tests
test: $(TARGET)
ifeq ($(OTEL_ENABLED),1)
	OTEL_SERVICE_NAME=cns_test ./$(TARGET) test all
else
	./$(TARGET) test all
endif

# Run SQL tests
test-sql: $(TARGET)
	@echo "Running 7T-SQL tests..."
	./$(TARGET) sql init
	./$(TARGET) sql create test_table id:int32 value:int32 score:float32
	./$(TARGET) sql insert test_table 1 100 95.5
	./$(TARGET) sql insert test_table 2 200 87.3
	./$(TARGET) sql insert test_table 3 150 92.1
	./$(TARGET) sql select "* FROM test_table WHERE value > 100"
	./$(TARGET) sql explain "SELECT * FROM test_table WHERE value = 150"
	./$(TARGET) sql show

# Run benchmarks
bench: $(TARGET)
ifeq ($(OTEL_ENABLED),1)
	OTEL_SERVICE_NAME=cns_bench ./$(TARGET) benchmark all
else
	./$(TARGET) bench all
endif

# Run SQL benchmarks
bench-sql: $(TARGET)
	@echo "Running 7T-SQL benchmarks..."
	./$(TARGET) sql init
	./$(TARGET) sql bench 10000

# Run gatekeeper
gate: $(GATEKEEPER_TARGET)
	./$(GATEKEEPER_TARGET)

# Run weaver
weave: $(WEAVER_TARGET)
	@echo "Running CNS weaver..."
	./$(WEAVER_TARGET) build/cns_spans.json templates/otel_header.h.j2 templates/otel_inject.c.j2 src
	@echo "✓ Generated OpenTelemetry instrumentation"

# Extract spans from TTL
extract-spans:
	@echo "Extracting spans from TTL ontology..."
	python3 codegen/extract_spans.py docs/ontology/cns-telemetry.ttl build/cns_spans.json
	@echo "✓ Extracted spans to build/cns_spans.json"

# Help
help:
	@echo "CNS Makefile targets:"
	@echo "  make         - Build CNS binary with 7T-SQL support"
	@echo "  make clean   - Remove build artifacts"
	@echo "  make install - Install to /usr/local/bin"
	@echo "  make test    - Run tests"
	@echo "  make test-sql - Run 7T-SQL specific tests"
	@echo "  make bench   - Run benchmarks"
	@echo "  make bench-sql - Run 7T-SQL benchmarks"
	@echo "  make gate    - Run gatekeeper validation"
	@echo "  make stubs   - Create stub domain files"

# Debug build
debug: CFLAGS += -g -DDEBUG -O0
debug: CXXFLAGS += -g -DDEBUG -O0
debug: clean $(TARGET)

# Profile build
profile: CFLAGS += -pg
profile: CXXFLAGS += -pg
profile: LDFLAGS += -pg
profile: clean $(TARGET)

# Build without OpenTelemetry
no-otel:
	$(MAKE) OTEL_ENABLED=0

# Run examples
run-examples: $(TARGET)
	@echo "Running CNS examples..."
	./$(TARGET) help
	./$(TARGET) version
	./$(TARGET) spin -n 100
	./$(TARGET) think "test pattern"
	./$(TARGET) reflect
ifeq ($(OTEL_ENABLED),1)
	./$(TARGET) benchmark -q hash
else
	./$(TARGET) bench -q hash
endif

.PHONY: all clean install uninstall test test-sql bench bench-sql gate weave extract-spans help stubs debug profile no-otel run-examples