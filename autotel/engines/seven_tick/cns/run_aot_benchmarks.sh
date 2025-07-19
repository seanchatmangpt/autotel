#!/bin/bash

# AOT Compiler 80/20 Benchmark Runner
# Tests compilation speed, generated code performance, memory usage, and incremental builds

set -e

echo "╔═══════════════════════════════════════════════════════════╗"
echo "║          AOT Compiler 80/20 Benchmark Suite               ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo ""

# Configuration
ITERATIONS="${1:-1000}"
REPORT_DIR="benchmark_results/aot_80_20"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Create report directory
mkdir -p "$REPORT_DIR"

# Function to check prerequisites
check_prerequisites() {
    echo "Checking prerequisites..."
    
    # Check for Python 3
    if ! command -v python3 &> /dev/null; then
        echo "ERROR: Python 3 is required but not found"
        exit 1
    fi
    
    # Check for RDFLib
    if ! python3 -c "import rdflib" &> /dev/null; then
        echo "WARNING: RDFLib not found. Installing..."
        pip3 install rdflib
    fi
    
    # Check for compiler
    if ! command -v clang &> /dev/null; then
        if ! command -v gcc &> /dev/null; then
            echo "ERROR: No C compiler found (clang or gcc required)"
            exit 1
        fi
    fi
    
    echo "✓ Prerequisites satisfied"
    echo ""
}

# Function to run compilation benchmarks
run_compilation_benchmarks() {
    echo "═══════════════════════════════════════════════════════════"
    echo "1. COMPILATION SPEED BENCHMARKS"
    echo "═══════════════════════════════════════════════════════════"
    echo ""
    
    # Test simple ontology compilation
    echo "Testing simple ontology compilation..."
    cat > simple_test.ttl << EOF
@prefix owl: <http://www.w3.org/2002/07/owl#> .
@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .
@prefix ex: <http://example.org/> .

ex:Person a owl:Class .
ex:Employee rdfs:subClassOf ex:Person .
ex:hasName a owl:DatatypeProperty ;
    rdfs:domain ex:Person ;
    rdfs:range xsd:string .
EOF
    
    # Time the compilation
    /usr/bin/time -l python3 codegen/aot_compiler.py \
        -i simple_test.ttl -o simple_out.h 2>&1 | tee "$REPORT_DIR/compile_simple_$TIMESTAMP.log"
    
    # Test complex ontology compilation
    echo ""
    echo "Testing complex ontology compilation..."
    cat > complex_test.ttl << EOF
@prefix owl: <http://www.w3.org/2002/07/owl#> .
@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .
@prefix ex: <http://example.org/> .

# Complex OWL with restrictions
ex:Person a owl:Class ;
    owl:equivalentClass [
        a owl:Restriction ;
        owl:onProperty ex:hasAge ;
        owl:minCardinality 1
    ] .

ex:Adult owl:intersectionOf (
    ex:Person
    [ a owl:Restriction ;
      owl:onProperty ex:hasAge ;
      owl:minInclusive 18 ]
) .

# Multiple inheritance
ex:Manager rdfs:subClassOf ex:Employee , ex:Leader .
ex:Employee rdfs:subClassOf ex:Person .
ex:Leader rdfs:subClassOf ex:Person .

# Property chains
ex:hasGrandparent owl:propertyChainAxiom ( ex:hasParent ex:hasParent ) .
EOF
    
    /usr/bin/time -l python3 codegen/aot_compiler.py \
        -i complex_test.ttl -o complex_out.h 2>&1 | tee "$REPORT_DIR/compile_complex_$TIMESTAMP.log"
    
    echo ""
    echo "Compilation benchmarks completed."
}

# Function to run generated code performance benchmarks
run_performance_benchmarks() {
    echo ""
    echo "═══════════════════════════════════════════════════════════"
    echo "2. GENERATED CODE PERFORMANCE BENCHMARKS"
    echo "═══════════════════════════════════════════════════════════"
    echo ""
    
    # Build and run the main benchmark
    make -f Makefile.aot_benchmark clean
    make -f Makefile.aot_benchmark benchmark ITERATIONS=$ITERATIONS
    
    # Copy results
    cp aot_benchmark_report.md "$REPORT_DIR/performance_report_$TIMESTAMP.md"
}

# Function to run memory usage benchmarks
run_memory_benchmarks() {
    echo ""
    echo "═══════════════════════════════════════════════════════════"
    echo "3. MEMORY USAGE BENCHMARKS"
    echo "═══════════════════════════════════════════════════════════"
    echo ""
    
    # Create large dataset
    echo "Creating large dataset (10K triples)..."
    python3 -c "
import rdflib
g = rdflib.Graph()
for i in range(10000):
    g.add((rdflib.URIRef(f'http://example.org/entity{i}'), 
           rdflib.RDF.type, 
           rdflib.URIRef(f'http://example.org/Class{i % 100}')))
g.serialize('large_dataset.ttl', format='turtle')
"
    
    # Measure memory usage
    echo "Measuring memory usage during compilation..."
    /usr/bin/time -l python3 codegen/aot_compiler.py \
        -i large_dataset.ttl -o large_out.h 2>&1 | tee "$REPORT_DIR/memory_large_$TIMESTAMP.log"
    
    # Extract memory stats
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS
        MAX_MEM=$(grep "maximum resident set size" "$REPORT_DIR/memory_large_$TIMESTAMP.log" | awk '{print $1/1024/1024}')
        echo "Peak memory usage: ${MAX_MEM} MB"
    else
        # Linux
        MAX_MEM=$(grep "Maximum resident set size" "$REPORT_DIR/memory_large_$TIMESTAMP.log" | awk '{print $6/1024}')
        echo "Peak memory usage: ${MAX_MEM} MB"
    fi
}

# Function to run incremental build benchmarks
run_incremental_benchmarks() {
    echo ""
    echo "═══════════════════════════════════════════════════════════"
    echo "4. INCREMENTAL BUILD BENCHMARKS"
    echo "═══════════════════════════════════════════════════════════"
    echo ""
    
    # Create base ontology
    echo "Creating base ontology..."
    cat > base.ttl << EOF
@prefix owl: <http://www.w3.org/2002/07/owl#> .
@prefix ex: <http://example.org/> .

ex:Person a owl:Class .
ex:Employee rdfs:subClassOf ex:Person .
EOF
    
    # Initial compilation
    echo "Initial compilation..."
    START_TIME=$(date +%s%N)
    python3 codegen/aot_compiler.py -i base.ttl -o base.h
    END_TIME=$(date +%s%N)
    INITIAL_TIME=$((($END_TIME - $START_TIME) / 1000000))
    echo "Initial compilation time: ${INITIAL_TIME}ms"
    
    # Add a new class
    echo ""
    echo "Adding new class..."
    echo "ex:Manager rdfs:subClassOf ex:Employee ." >> base.ttl
    
    # Incremental compilation
    echo "Incremental compilation..."
    START_TIME=$(date +%s%N)
    python3 codegen/aot_compiler.py -i base.ttl -o base.h --incremental
    END_TIME=$(date +%s%N)
    INCREMENTAL_TIME=$((($END_TIME - $START_TIME) / 1000000))
    echo "Incremental compilation time: ${INCREMENTAL_TIME}ms"
    
    # Calculate speedup
    if [ $INCREMENTAL_TIME -gt 0 ]; then
        SPEEDUP=$(echo "scale=2; $INITIAL_TIME / $INCREMENTAL_TIME" | bc)
        echo "Incremental build speedup: ${SPEEDUP}x"
    fi
}

# Function to run error handling benchmarks
run_error_benchmarks() {
    echo ""
    echo "═══════════════════════════════════════════════════════════"
    echo "5. ERROR HANDLING BENCHMARKS"
    echo "═══════════════════════════════════════════════════════════"
    echo ""
    
    # Test malformed Turtle
    echo "Testing malformed Turtle syntax..."
    cat > malformed.ttl << EOF
@prefix ex: <http://example.org/> .
ex:Bad [ [ nested without closing
EOF
    
    python3 codegen/aot_compiler.py -i malformed.ttl -o error1.h 2>&1 | \
        tee "$REPORT_DIR/error_malformed_$TIMESTAMP.log" || true
    
    # Test circular dependencies
    echo ""
    echo "Testing circular dependencies..."
    cat > circular.ttl << EOF
@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .
@prefix ex: <http://example.org/> .

ex:A rdfs:subClassOf ex:B .
ex:B rdfs:subClassOf ex:C .
ex:C rdfs:subClassOf ex:A .
EOF
    
    python3 codegen/aot_compiler.py -i circular.ttl -o error2.h 2>&1 | \
        tee "$REPORT_DIR/error_circular_$TIMESTAMP.log" || true
}

# Function to generate final report
generate_final_report() {
    echo ""
    echo "═══════════════════════════════════════════════════════════"
    echo "GENERATING FINAL REPORT"
    echo "═══════════════════════════════════════════════════════════"
    echo ""
    
    FINAL_REPORT="$REPORT_DIR/final_report_$TIMESTAMP.md"
    
    cat > "$FINAL_REPORT" << EOF
# AOT Compiler 80/20 Benchmark Results

**Date**: $(date)
**Iterations**: $ITERATIONS

## Executive Summary

This report presents comprehensive benchmarks of the AOT (Ahead-of-Time) compiler, 
testing the 80/20 principle where 80% of queries follow common patterns and 20% 
represent edge cases.

## 1. Compilation Speed

### Simple Ontology (80% case)
$(grep "real" "$REPORT_DIR/compile_simple_$TIMESTAMP.log" 2>/dev/null || echo "N/A")

### Complex Ontology (20% case)
$(grep "real" "$REPORT_DIR/compile_complex_$TIMESTAMP.log" 2>/dev/null || echo "N/A")

## 2. Generated Code Performance

$(grep -A 10 "Summary" aot_benchmark_report.md 2>/dev/null || echo "See performance_report_$TIMESTAMP.md")

## 3. Memory Usage

Large dataset (10K triples): ${MAX_MEM:-N/A} MB peak memory

## 4. Incremental Builds

- Initial compilation: ${INITIAL_TIME:-N/A}ms
- Incremental compilation: ${INCREMENTAL_TIME:-N/A}ms
- Speedup: ${SPEEDUP:-N/A}x

## 5. Error Handling

- Malformed syntax: $(grep -c "ERROR" "$REPORT_DIR/error_malformed_$TIMESTAMP.log" 2>/dev/null || echo "0") errors detected
- Circular dependencies: $(grep -c "ERROR" "$REPORT_DIR/error_circular_$TIMESTAMP.log" 2>/dev/null || echo "0") errors detected

## Recommendations

Based on the 80/20 analysis:

1. **Optimize for common cases**: Focus on simple ontologies and basic SHACL shapes
2. **Cache compilation results**: Leverage incremental builds for large projects
3. **Memory efficiency**: Use streaming parsers for very large datasets
4. **Error recovery**: Implement graceful degradation for edge cases

## Files Generated

- Performance report: performance_report_$TIMESTAMP.md
- Compilation logs: compile_*_$TIMESTAMP.log
- Memory analysis: memory_*_$TIMESTAMP.log
- Error logs: error_*_$TIMESTAMP.log
EOF
    
    echo "Final report saved to: $FINAL_REPORT"
}

# Main execution
main() {
    check_prerequisites
    
    # Run all benchmark categories
    run_compilation_benchmarks
    run_performance_benchmarks
    run_memory_benchmarks
    run_incremental_benchmarks
    run_error_benchmarks
    
    # Generate final report
    generate_final_report
    
    # Cleanup temporary files
    rm -f *.ttl *.h *.sql *.rq
    
    echo ""
    echo "╔═══════════════════════════════════════════════════════════╗"
    echo "║            Benchmark Suite Completed                      ║"
    echo "╚═══════════════════════════════════════════════════════════╝"
    echo ""
    echo "Results saved to: $REPORT_DIR/"
    echo ""
    
    # Show summary
    echo "Key Results:"
    echo "------------"
    grep -E "(Tests passed|Weighted average|Business coverage|Peak memory)" "$FINAL_REPORT" || true
}

# Run main function
main "$@"