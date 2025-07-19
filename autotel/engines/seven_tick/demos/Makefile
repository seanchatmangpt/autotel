# Demo Makefile for 7T Engine
# Builds all demo programs showcasing different capabilities

CC = gcc
CFLAGS = -O3 -march=native -Wall -Wextra
INCLUDES = -I../runtime/src -I../compiler/src
LIBS = -lm

# Demo targets
DEMOS = 01_sparql_knowledge_graph \
        02_shacl_validation \
        03_cjinja_templating \
        04_memory_optimization \
        05_integrated_workflow

# Runtime and compiler sources
RUNTIME_SRC = ../runtime/src/seven_t_runtime.c
COMPILER_SRC = ../compiler/src/cjinja.c

.PHONY: all clean run-all

all: $(DEMOS)

# SPARQL Knowledge Graph Demo
01_sparql_knowledge_graph: 01_sparql_knowledge_graph.c $(RUNTIME_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(RUNTIME_SRC) $(LIBS)

# SHACL Validation Demo
02_shacl_validation: 02_shacl_validation.c $(RUNTIME_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(RUNTIME_SRC) $(LIBS)

# CJinja Templating Demo
03_cjinja_templating: 03_cjinja_templating.c $(COMPILER_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(COMPILER_SRC) $(LIBS)

# Memory Optimization Demo
04_memory_optimization: 04_memory_optimization.c $(RUNTIME_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(RUNTIME_SRC) $(LIBS)

# Integrated Workflow Demo
05_integrated_workflow: 05_integrated_workflow.c $(RUNTIME_SRC) $(COMPILER_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(RUNTIME_SRC) $(COMPILER_SRC) $(LIBS)

# Run all demos
run-all: $(DEMOS)
	@echo "🚀 Running all 7T Engine demos..."
	@echo "=================================="
	@echo ""
	@for demo in $(DEMOS); do \
		echo "Running $$demo..."; \
		echo "-------------------"; \
		./$$demo; \
		echo ""; \
		echo "Press Enter to continue to next demo..."; \
		read; \
		echo ""; \
	done
	@echo "✅ All demos completed!"

# Run individual demos
run-sparql: 01_sparql_knowledge_graph
	./01_sparql_knowledge_graph

run-shacl: 02_shacl_validation
	./02_shacl_validation

run-cjinja: 03_cjinja_templating
	./03_cjinja_templating

run-memory: 04_memory_optimization
	./04_memory_optimization

run-integrated: 05_integrated_workflow
	./05_integrated_workflow

# Clean build artifacts
clean:
	rm -f $(DEMOS)
	rm -f *.o

# Help target
help:
	@echo "7T Engine Demo Makefile"
	@echo "======================="
	@echo ""
	@echo "Available targets:"
	@echo "  all              - Build all demo programs"
	@echo "  run-all          - Run all demos sequentially"
	@echo "  run-sparql       - Run SPARQL knowledge graph demo"
	@echo "  run-shacl        - Run SHACL validation demo"
	@echo "  run-cjinja       - Run CJinja templating demo"
	@echo "  run-memory       - Run memory optimization demo"
	@echo "  run-integrated   - Run integrated workflow demo"
	@echo "  clean            - Remove build artifacts"
	@echo "  help             - Show this help message"
	@echo ""
	@echo "Demo descriptions:"
	@echo "  01_sparql_knowledge_graph - SPARQL pattern matching and queries"
	@echo "  02_shacl_validation       - SHACL constraint validation"
	@echo "  03_cjinja_templating      - CJinja template rendering"
	@echo "  04_memory_optimization    - Cache and memory optimization"
	@echo "  05_integrated_workflow    - All components working together" 