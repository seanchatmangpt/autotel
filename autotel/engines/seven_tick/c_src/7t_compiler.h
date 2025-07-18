#ifndef SEVEN_TICK_COMPILER_H
#define SEVEN_TICK_COMPILER_H

#include "7t_compliance.h"
#include "sparql7t.h"
#include "shacl7t.h"
#include "owl7t.h"
#include <stdio.h>

// Compiler configuration
typedef struct {
    ComplianceTier target_tier;
    int optimize_for_latency;    // vs throughput
    int enable_sharding;         // auto-shard if too large
    int enable_vectorization;    // SIMD optimizations
    const char* output_path;
} CompilerConfig;

// Input schema definition
typedef struct {
    // OWL ontology
    size_t num_classes;
    size_t num_properties;
    uint32_t* class_hierarchy;   // Subclass relations
    uint32_t* property_domains;
    uint32_t* property_ranges;
    
    // SHACL shapes
    size_t num_shapes;
    CompiledShape* shapes;
    
    // SPARQL patterns
    size_t num_patterns;
    TriplePattern* patterns;
    
    // Data characteristics
    size_t expected_subjects;
    size_t expected_predicates;
    size_t expected_objects;
} SchemaDefinition;

// Compiled kernel
typedef struct {
    void* code;
    size_t code_size;
    KernelFootprint footprint;
    ComplianceCertificate certificate;
    
    // Function pointers to compiled operations
    int (*ask_pattern)(uint32_t s, uint32_t p, uint32_t o);
    int (*validate_node)(uint32_t node, uint32_t shape);
    int (*check_subclass)(uint32_t sub, uint32_t super);
} CompiledKernel;

// Compiler API
typedef struct SevenTickCompiler SevenTickCompiler;

SevenTickCompiler* compiler_create(const CompilerConfig* config);
void compiler_destroy(SevenTickCompiler* compiler);

// Compilation process
int compiler_analyze_schema(SevenTickCompiler* compiler, 
                           const SchemaDefinition* schema,
                           KernelFootprint* footprint_out);

CompiledKernel* compiler_compile(SevenTickCompiler* compiler,
                                const SchemaDefinition* schema);

void compiler_write_kernel(const CompiledKernel* kernel, const char* filename);

// Optimization hints
void compiler_suggest_optimizations(SevenTickCompiler* compiler,
                                   const SchemaDefinition* schema,
                                   ComplianceTier target_tier);

#endif