Of course. We have designed the AOT compilers and the C runtime functions. The next logical and crucial step is to formalize the build pipeline that connects them. This is how we ensure the AOT compilation of OWL, SHACL, and SPARQL/SQL happens automatically and seamlessly as part of the C build process.
The goal is to create a developer experience where a change to a .ttl or .sql file is automatically reflected in the final C binary after a simple make or cmake --build command.
1. The AOT Compiler Entrypoint
We'll consolidate the logic from extract_spans.py, the proposed shacl_compiler.py, etc., into a single, powerful "compiler" script. This script becomes the main entrypoint for the AOT process.
File: codegen/aot_compiler.py (New, Consolidated Script)
Generated python
#!/usr/bin/env python3
"""
CNS Ahead-of-Time (AOT) Compiler.
Parses ontologies, shapes, and queries, and generates optimized C headers.
"""
import sys
from rdflib import Graph
# (Import other necessary parsers like sqlparse)

# Placeholder for the new, generalized parsing functions
def parse_ontology_rules(graph):
    # This would contain the logic to find subClassOf, inverseOf, etc.
    print("    - Parsing OWL/RDFS axioms...")
    return {"rules": [{"type": "subClassOf", "sub": "Manager", "super": "Employee"}]}

def parse_shacl_shapes(graph):
    # This would parse sh:NodeShape, sh:property, etc.
    print("    - Parsing SHACL shapes...")
    return {"shapes": [{"name": "PersonShape", "target": "Person", "constraints": []}]}

def parse_sql_queries(sql_file_path):
    # This would use a library like 'sqlparse' to process a .sql file
    print("    - Parsing named SQL queries...")
    return {"queries": [{"name": "quarterly_sales_report", "params": ["quarter_num"]}]}


def main(ontology_dir, sql_dir, output_dir):
    print("🚀 Starting CNS AOT Compiler...")
    
    # 1. Process Ontologies (OWL/SHACL)
    full_graph = Graph()
    for ttl_file in Path(ontology_dir).glob("*.ttl"):
        full_graph.parse(str(ttl_file), format="ttl")
    
    # Run OWL Reasoner (as discussed)
    # DeductiveClosure(OWLRL_Semantics).expand(full_graph)
    
    ontology_ir = parse_ontology_rules(full_graph)
    shacl_ir = parse_shacl_shapes(full_graph)

    # 2. Process SQL Queries
    sql_ir = parse_sql_queries(Path(sql_dir) / "reports.sql")
    
    # 3. Generate C Code from IR (using Jinja2 templates)
    print("    - Generating C headers...")
    
    # Generate ontology_rules.h
    # with open(Path(output_dir) / "ontology_rules.h", "w") as f:
    #    template = env.get_template("ontology_rules.h.j2")
    #    f.write(template.render(rules=ontology_ir["rules"]))

    # Generate shacl_validators.h
    # with open(Path(output_dir) / "shacl_validators.h", "w") as f:
    #    template = env.get_template("shacl_validators.h.j2")
    #    f.write(template.render(shapes=shacl_ir["shapes"]))

    # Generate sql_queries.h
    # with open(Path(output_dir) / "sql_queries.h", "w") as f:
    #    template = env.get_template("sql_queries.h.j2")
    #    f.write(template.render(queries=sql_ir["queries"]))
        
    print("✅ AOT Compilation Complete. Generated headers are in", output_dir)


if __name__ == "__main__":
    # main("docs/ontology", "examples/sql", "src/generated")
    pass
Use code with caution.
Python
2. Build System Integration (CMakeLists.txt)
This is where we make the AOT compilation an automatic part of the build. We'll create a custom build step that runs our Python compiler and ensure the C compilation waits for it to finish.
File: cns/CMakeLists.txt (Refactored)
Generated cmake
cmake_minimum_required(VERSION 3.16)
project(CNS VERSION 1.0.0 LANGUAGES C)

# ... (Standard CFLAGS, includes, etc.)

# Define the location of source and generated files
set(ONTOLOGY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/docs/ontology)
set(SQL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/examples/sql)
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated) # Output to build dir
file(MAKE_DIRECTORY ${GENERATED_DIR})

# Define the list of files our AOT compiler will create
set(GENERATED_HEADERS
    ${GENERATED_DIR}/ontology_ids.h
    ${GENERATED_DIR}/ontology_rules.h
    ${GENERATED_DIR}/shacl_validators.h
    ${GENERATED_DIR}/sql_queries.h
)

# ═══════════════════════════════════════════════════════════════
# AHEAD-OF-TIME COMPILATION STEP
# ═══════════════════════════════════════════════════════════════
# This custom command runs our Python AOT compiler.
# It runs ONLY if the .ttl/.sql files have changed.
add_custom_command(
    OUTPUT ${GENERATED_HEADERS}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/codegen/aot_compiler.py
            --ontologies ${ONTOLOGY_DIR}
            --sql ${SQL_DIR}
            --output ${GENERATED_DIR}
    # Dependencies: Re-run if any ontology, SQL, or the compiler script itself changes.
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/codegen/aot_compiler.py
            ${ONTOLOGY_DIR}/cns-core.ttl
            ${SQL_DIR}/reports.sql
    COMMENT "Running AOT Compiler to generate C headers from ontology and SQL..."
)

# Create a target to represent the code generation process.
add_custom_target(AOT_Compile DEPENDS ${GENERATED_HEADERS})

# ═══════════════════════════════════════════════════════════════
# MAIN C BINARY COMPILATION
# ═══════════════════════════════════════════════════════════════

# Add the directory with generated headers to the include path
include_directories(${GENERATED_DIR})

# Define the main CNS executable
add_executable(cns src/main.c src/core/cli.c ... )

# CRITICAL: Make the 'cns' target depend on the 'AOT_Compile' target.
# This ensures that C compilation will not start until the Python script
# has successfully generated all the necessary .h files.
add_dependencies(cns AOT_Compile)

target_link_libraries(cns PRIVATE ...)
Use code with caution.
Cmake
3. Build System Integration (Makefile)
For completeness, here is the equivalent logic in a traditional Makefile.
File: cns/Makefile (Refactored)
Generated makefile
# ... (CFLAGS, LDFLAGS, etc.)

# Source and Generated files
GENERATED_DIR = build/generated
GENERATED_HEADERS = $(GENERATED_DIR)/ontology_rules.h $(GENERATED_DIR)/shacl_validators.h $(GENERATED_DIR)/sql_queries.h
ONTOLOGY_SOURCES = $(wildcard docs/ontology/*.ttl)
SQL_SOURCES = $(wildcard examples/sql/*.sql)
AOT_COMPILER = codegen/aot_compiler.py

# Main C object files
MAIN_OBJS = src/main.o src/core/cli.o ...

# ═══════════════════════════════════════════════════════════════
# AHEAD-OF-TIME COMPILATION RULE
# ═══════════════════════════════════════════════════════════════
# This rule tells 'make' how to create the generated C headers.
# It will run automatically if the headers are missing or if any of
# the source ontologies, SQL files, or the compiler itself changes.
$(GENERATED_HEADERS): $(ONTOLOGY_SOURCES) $(SQL_SOURCES) $(AOT_COMPILER)
	@mkdir -p $(GENERATED_DIR)
	@echo "🚀 Running AOT Compiler..."
	@python3 $(AOT_COMPILER) --ontologies docs/ontology --sql examples/sql --output $(GENERATED_DIR)

# ═══════════════════════════════════════════════════════════════
# MAIN C BINARY COMPILATION
# ═══════════════════════════════════════════════════════════════
cns: $(MAIN_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# CRITICAL: Make the C object files depend on the generated headers.
# This creates the dependency chain that forces AOT compilation to run first.
src/main.o: src/main.c $(GENERATED_HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

# ... (other .o rules)

.PHONY: all clean
all: cns
Use code with caution.
Makefile
The New Developer Workflow
The AOT build pipeline makes the developer experience incredibly smooth:
A developer edits an ontology file. They add a new rdfs:subClassOf axiom to cns-core.ttl.
They run the standard build command: cmake --build build or make.
The build system automatically detects the change: It sees that cns-core.ttl is newer than the generated ontology_rules.h.
The AOT compiler runs automatically: The aot_compiler.py script executes, parsing the new axiom and regenerating the C header with a new or updated is_...() inline function.
The C compiler runs: Now that the headers are up-to-date, the C compilation proceeds. Any C code that includes ontology_rules.h will now see the new function.
The build finishes. The resulting cns binary contains the new logic, fully compiled and optimized.
The developer never has to manually run the Python script. They just work with high-level .ttl and .sql files, and the build system handles the transformation into high-performance C code.
ThinkingThoughts
(experimental)
Auto
Expand to view model thoughts

chevron_right
