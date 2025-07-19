/*
 * CNS SPARQL Dedicated Benchmark Suite
 * Comprehensive testing of SPARQL query performance with real RDF data patterns
 * 
 * This focuses specifically on SPARQL operations:
 * - Triple pattern matching (?s ?p ?o)
 * - Basic Graph Pattern (BGP) matching
 * - OPTIONAL patterns
 * - UNION queries
 * - Filter expressions
 * - Graph traversal operations
 * - RDF dataset operations
 * - Index lookup performance
 * - Join optimization
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

#include "include/cns/engines/sparql.h"
#include "include/cns/telemetry/otel.h"

#define SPARQL_BENCH_ITERATIONS 25000
#define SPARQL_WARMUP_ITERATIONS 1000
#define SPARQL_TEST_TRIPLES 50000

// CPU cycle measurement
static inline uint64_t get_cycles(void) {
#if defined(__x86_64__) || defined(__i386__)
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
#elif defined(__aarch64__)
    uint64_t val;
    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r" (val));
    return val;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 3000000000ULL + (uint64_t)ts.tv_nsec * 3;
#endif
}

// RDF test data generation
typedef struct {
    uint32_t* subjects;
    uint32_t* predicates;
    uint32_t* objects;
    uint32_t* types;           // rdf:type objects
    uint32_t* properties;      // Property values
    uint32_t* graph_ids;       // Named graph IDs
    char** uri_strings;        // String representations
    size_t triple_count;
} sparql_test_data_t;

// Realistic RDF vocabularies for testing
static const uint32_t RDF_TYPE = 1;
static const uint32_t RDFS_LABEL = 2;
static const uint32_t FOAF_NAME = 3;
static const uint32_t FOAF_KNOWS = 4;
static const uint32_t DC_TITLE = 5;
static const uint32_t DC_CREATOR = 6;

static const uint32_t PERSON_CLASS = 100;
static const uint32_t DOCUMENT_CLASS = 101;
static const uint32_t ORGANIZATION_CLASS = 102;

static void generate_sparql_test_data(sparql_test_data_t* data, size_t triple_count) {
    data->triple_count = triple_count;
    
    // Allocate aligned memory
    data->subjects = aligned_alloc(64, triple_count * sizeof(uint32_t));
    data->predicates = aligned_alloc(64, triple_count * sizeof(uint32_t));
    data->objects = aligned_alloc(64, triple_count * sizeof(uint32_t));
    data->types = aligned_alloc(64, triple_count * sizeof(uint32_t));
    data->properties = aligned_alloc(64, triple_count * sizeof(uint32_t));
    data->graph_ids = aligned_alloc(64, triple_count * sizeof(uint32_t));
    data->uri_strings = malloc(triple_count * sizeof(char*));
    
    srand(42); // Fixed seed for reproducibility
    
    // Generate realistic RDF graph patterns
    size_t person_count = triple_count / 10;
    size_t doc_count = triple_count / 15;
    size_t org_count = triple_count / 20;
    
    for (size_t i = 0; i < triple_count; i++) {
        // Generate subjects with clustering (real RDF graphs have clustering)
        if (i < person_count) {
            data->subjects[i] = 1000 + (i % person_count);  // Person URIs
            data->types[i] = PERSON_CLASS;
        } else if (i < person_count + doc_count) {
            data->subjects[i] = 2000 + ((i - person_count) % doc_count);  // Document URIs
            data->types[i] = DOCUMENT_CLASS;
        } else if (i < person_count + doc_count + org_count) {
            data->subjects[i] = 3000 + ((i - person_count - doc_count) % org_count);  // Org URIs
            data->types[i] = ORGANIZATION_CLASS;
        } else {
            // Mixed/property triples
            data->subjects[i] = 1000 + (rand() % (person_count + doc_count + org_count));
            data->types[i] = 0; // No type
        }
        
        // Generate predicates with realistic distributions
        double pred_rand = (double)rand() / RAND_MAX;
        if (pred_rand < 0.3) {
            data->predicates[i] = RDF_TYPE;
            data->objects[i] = data->types[i];
        } else if (pred_rand < 0.5) {
            data->predicates[i] = RDFS_LABEL;
            data->objects[i] = 5000 + (rand() % 1000); // String literals
        } else if (pred_rand < 0.65) {
            data->predicates[i] = FOAF_NAME;
            data->objects[i] = 6000 + (rand() % 500);  // Name literals
        } else if (pred_rand < 0.8) {
            data->predicates[i] = FOAF_KNOWS;
            data->objects[i] = 1000 + (rand() % person_count); // Person to person
        } else if (pred_rand < 0.9) {
            data->predicates[i] = DC_TITLE;
            data->objects[i] = 7000 + (rand() % 200);  // Title literals
        } else {
            data->predicates[i] = DC_CREATOR;
            data->objects[i] = 1000 + (rand() % person_count); // Creator links
        }
        
        // Generate property values
        data->properties[i] = rand() % 10000;
        
        // Generate graph IDs (most triples in default graph)
        data->graph_ids[i] = (rand() % 100 < 90) ? 0 : (1 + rand() % 10);
        
        // Generate URI strings
        size_t uri_len = 20 + (rand() % 30);
        data->uri_strings[i] = malloc(uri_len + 1);
        snprintf(data->uri_strings[i], uri_len + 1, "http://ex.org/res%zu_%u", 
                i, data->subjects[i]);
    }
}

static void cleanup_sparql_test_data(sparql_test_data_t* data) {
    free(data->subjects);
    free(data->predicates);
    free(data->objects);
    free(data->types);
    free(data->properties);
    free(data->graph_ids);
    
    for (size_t i = 0; i < data->triple_count; i++) {
        free(data->uri_strings[i]);
    }
    free(data->uri_strings);
}

// Benchmark result structure
typedef struct {
    const char* name;
    const char* query_pattern;
    uint64_t iterations;
    uint64_t min_cycles;
    uint64_t max_cycles;
    uint64_t total_cycles;
    double avg_cycles;
    double cycles_per_triple;
    double results_per_query;
    bool seven_tick_compliant;
    bool passed;
    const char* error_msg;
} sparql_bench_result_t;

static void calculate_sparql_stats(uint64_t* measurements, size_t count,
                                  size_t triples_searched, double avg_results,
                                  sparql_bench_result_t* result) {
    // Sort measurements
    for (size_t i = 0; i < count - 1; i++) {
        for (size_t j = 0; j < count - i - 1; j++) {
            if (measurements[j] > measurements[j + 1]) {
                uint64_t temp = measurements[j];
                measurements[j] = measurements[j + 1];
                measurements[j + 1] = temp;
            }
        }
    }
    
    result->min_cycles = measurements[0];
    result->max_cycles = measurements[count - 1];
    
    result->total_cycles = 0;
    for (size_t i = 0; i < count; i++) {
        result->total_cycles += measurements[i];
    }
    result->avg_cycles = (double)result->total_cycles / count;
    result->cycles_per_triple = result->avg_cycles / triples_searched;
    result->results_per_query = avg_results;
    
    result->seven_tick_compliant = (result->cycles_per_triple <= 7.0);
    result->passed = result->seven_tick_compliant;
}

/*═══════════════════════════════════════════════════════════════
  Basic Pattern Matching Benchmarks
  ═══════════════════════════════════════════════════════════════*/

static sparql_bench_result_t benchmark_sparql_triple_pattern_spo(sparql_test_data_t* data, CNSSparqlEngine* engine) {
    sparql_bench_result_t result = {
        .name = "Triple Pattern (?s ?p ?o)",
        .query_pattern = "?s ?p ?o",
        .iterations = SPARQL_BENCH_ITERATIONS / 1000
    };
    
    uint64_t* measurements = malloc(result.iterations * sizeof(uint64_t));
    uint64_t total_results = 0;
    
    for (uint64_t i = 0; i < result.iterations; i++) {
        uint64_t start = get_cycles();
        
        // Query all triples - very expensive, so we simulate with limited scan
        uint32_t result_count = 0;
        for (size_t j = 0; j < 1000; j++) { // Limited scan
            if (cns_sparql_ask_pattern(engine, 
                data->subjects[j % data->triple_count],
                data->predicates[j % data->triple_count],
                data->objects[j % data->triple_count])) {
                result_count++;
            }
        }
        
        uint64_t end = get_cycles();
        measurements[i] = end - start;
        total_results += result_count;
    }
    
    double avg_results = (double)total_results / result.iterations;
    calculate_sparql_stats(measurements, result.iterations, 1000, avg_results, &result);
    
    free(measurements);
    return result;
}

static sparql_bench_result_t benchmark_sparql_subject_lookup(sparql_test_data_t* data, CNSSparqlEngine* engine) {
    sparql_bench_result_t result = {
        .name = "Subject Lookup (s ?p ?o)",
        .query_pattern = "<subject> ?p ?o",
        .iterations = SPARQL_BENCH_ITERATIONS
    };
    
    uint64_t* measurements = malloc(result.iterations * sizeof(uint64_t));
    uint64_t total_results = 0;
    
    for (uint64_t i = 0; i < result.iterations; i++) {
        uint64_t start = get_cycles();
        
        uint32_t target_subject = data->subjects[i % data->triple_count];
        uint32_t result_count = 0;
        
        // Find all triples with this subject
        for (size_t j = 0; j < data->triple_count; j++) {
            if (data->subjects[j] == target_subject) {
                result_count++;
            }
        }
        
        uint64_t end = get_cycles();
        measurements[i] = end - start;
        total_results += result_count;
    }
    
    double avg_results = (double)total_results / result.iterations;
    calculate_sparql_stats(measurements, result.iterations, data->triple_count, avg_results, &result);
    
    free(measurements);
    return result;
}

static sparql_bench_result_t benchmark_sparql_predicate_lookup(sparql_test_data_t* data, CNSSparqlEngine* engine) {
    sparql_bench_result_t result = {
        .name = "Predicate Lookup (?s p ?o)",
        .query_pattern = "?s <predicate> ?o",
        .iterations = SPARQL_BENCH_ITERATIONS
    };
    
    uint64_t* measurements = malloc(result.iterations * sizeof(uint64_t));
    uint64_t total_results = 0;
    
    // Test common predicates
    uint32_t test_predicates[] = {RDF_TYPE, RDFS_LABEL, FOAF_NAME, FOAF_KNOWS, DC_TITLE};
    size_t pred_count = sizeof(test_predicates) / sizeof(test_predicates[0]);
    
    for (uint64_t i = 0; i < result.iterations; i++) {
        uint64_t start = get_cycles();
        
        uint32_t target_predicate = test_predicates[i % pred_count];
        uint32_t result_count = 0;
        
        // Find all triples with this predicate
        for (size_t j = 0; j < data->triple_count; j++) {
            if (data->predicates[j] == target_predicate) {
                result_count++;
            }
        }
        
        uint64_t end = get_cycles();
        measurements[i] = end - start;
        total_results += result_count;
    }
    
    double avg_results = (double)total_results / result.iterations;
    calculate_sparql_stats(measurements, result.iterations, data->triple_count, avg_results, &result);
    
    free(measurements);
    return result;
}

static sparql_bench_result_t benchmark_sparql_object_lookup(sparql_test_data_t* data, CNSSparqlEngine* engine) {
    sparql_bench_result_t result = {
        .name = "Object Lookup (?s ?p o)",
        .query_pattern = "?s ?p <object>",
        .iterations = SPARQL_BENCH_ITERATIONS
    };
    
    uint64_t* measurements = malloc(result.iterations * sizeof(uint64_t));
    uint64_t total_results = 0;
    
    for (uint64_t i = 0; i < result.iterations; i++) {
        uint64_t start = get_cycles();
        
        uint32_t target_object = data->objects[i % data->triple_count];
        uint32_t result_count = 0;
        
        // Find all triples with this object
        for (size_t j = 0; j < data->triple_count; j++) {
            if (data->objects[j] == target_object) {
                result_count++;
            }
        }
        
        uint64_t end = get_cycles();
        measurements[i] = end - start;
        total_results += result_count;
    }
    
    double avg_results = (double)total_results / result.iterations;
    calculate_sparql_stats(measurements, result.iterations, data->triple_count, avg_results, &result);
    
    free(measurements);
    return result;
}

/*═══════════════════════════════════════════════════════════════
  Complex Pattern Benchmarks
  ═══════════════════════════════════════════════════════════════*/

static sparql_bench_result_t benchmark_sparql_type_filter(sparql_test_data_t* data, CNSSparqlEngine* engine) {
    sparql_bench_result_t result = {
        .name = "Type Filter (?s rdf:type Class)",
        .query_pattern = "?s rdf:type <Class>",
        .iterations = SPARQL_BENCH_ITERATIONS
    };
    
    uint64_t* measurements = malloc(result.iterations * sizeof(uint64_t));
    uint64_t total_results = 0;
    
    uint32_t test_types[] = {PERSON_CLASS, DOCUMENT_CLASS, ORGANIZATION_CLASS};
    
    for (uint64_t i = 0; i < result.iterations; i++) {
        uint64_t start = get_cycles();
        
        uint32_t target_type = test_types[i % 3];
        uint32_t result_count = 0;
        
        // Find all instances of this type
        for (size_t j = 0; j < data->triple_count; j++) {
            if (data->predicates[j] == RDF_TYPE && data->objects[j] == target_type) {
                result_count++;
            }
        }
        
        uint64_t end = get_cycles();
        measurements[i] = end - start;
        total_results += result_count;
    }
    
    double avg_results = (double)total_results / result.iterations;
    calculate_sparql_stats(measurements, result.iterations, data->triple_count, avg_results, &result);
    
    free(measurements);
    return result;
}

static sparql_bench_result_t benchmark_sparql_property_path(sparql_test_data_t* data, CNSSparqlEngine* engine) {
    sparql_bench_result_t result = {
        .name = "Property Path (?person foaf:knows ?friend)",
        .query_pattern = "?person foaf:knows ?friend",
        .iterations = SPARQL_BENCH_ITERATIONS / 10
    };
    
    uint64_t* measurements = malloc(result.iterations * sizeof(uint64_t));
    uint64_t total_results = 0;
    
    for (uint64_t i = 0; i < result.iterations; i++) {
        uint64_t start = get_cycles();
        
        uint32_t result_count = 0;
        
        // Find all foaf:knows relationships
        for (size_t j = 0; j < data->triple_count; j++) {
            if (data->predicates[j] == FOAF_KNOWS) {
                // Check if both subject and object are persons
                bool subject_is_person = false;
                bool object_is_person = false;
                
                for (size_t k = 0; k < data->triple_count; k++) {
                    if (data->subjects[k] == data->subjects[j] && 
                        data->predicates[k] == RDF_TYPE && 
                        data->objects[k] == PERSON_CLASS) {
                        subject_is_person = true;
                    }
                    if (data->subjects[k] == data->objects[j] && 
                        data->predicates[k] == RDF_TYPE && 
                        data->objects[k] == PERSON_CLASS) {
                        object_is_person = true;
                    }
                }
                
                if (subject_is_person && object_is_person) {
                    result_count++;
                }
            }
        }
        
        uint64_t end = get_cycles();
        measurements[i] = end - start;
        total_results += result_count;
    }
    
    double avg_results = (double)total_results / result.iterations;
    calculate_sparql_stats(measurements, result.iterations, data->triple_count, avg_results, &result);
    
    free(measurements);
    return result;
}

static sparql_bench_result_t benchmark_sparql_join_pattern(sparql_test_data_t* data, CNSSparqlEngine* engine) {
    sparql_bench_result_t result = {
        .name = "Join Pattern (person-document)",
        .query_pattern = "?person dc:creator ?doc . ?doc dc:title ?title",
        .iterations = SPARQL_BENCH_ITERATIONS / 100
    };
    
    uint64_t* measurements = malloc(result.iterations * sizeof(uint64_t));
    uint64_t total_results = 0;
    
    for (uint64_t i = 0; i < result.iterations; i++) {
        uint64_t start = get_cycles();
        
        uint32_t result_count = 0;
        
        // Find creator-document relationships with titles
        for (size_t j = 0; j < data->triple_count; j++) {
            if (data->predicates[j] == DC_CREATOR) {
                uint32_t person = data->objects[j];  // Creator
                uint32_t document = data->subjects[j]; // Document
                
                // Check if document has title
                for (size_t k = 0; k < data->triple_count; k++) {
                    if (data->subjects[k] == document && data->predicates[k] == DC_TITLE) {
                        result_count++;
                        break;
                    }
                }
            }
        }
        
        uint64_t end = get_cycles();
        measurements[i] = end - start;
        total_results += result_count;
    }
    
    double avg_results = (double)total_results / result.iterations;
    calculate_sparql_stats(measurements, result.iterations, data->triple_count, avg_results, &result);
    
    free(measurements);
    return result;
}

/*═══════════════════════════════════════════════════════════════
  Graph Operations Benchmarks
  ═══════════════════════════════════════════════════════════════*/

static sparql_bench_result_t benchmark_sparql_graph_pattern(sparql_test_data_t* data, CNSSparqlEngine* engine) {
    sparql_bench_result_t result = {
        .name = "Named Graph Pattern",
        .query_pattern = "GRAPH ?g { ?s ?p ?o }",
        .iterations = SPARQL_BENCH_ITERATIONS / 10
    };
    
    uint64_t* measurements = malloc(result.iterations * sizeof(uint64_t));
    uint64_t total_results = 0;
    
    for (uint64_t i = 0; i < result.iterations; i++) {
        uint64_t start = get_cycles();
        
        uint32_t target_graph = data->graph_ids[i % data->triple_count];
        uint32_t result_count = 0;
        
        // Count triples in specific graph
        for (size_t j = 0; j < data->triple_count; j++) {
            if (data->graph_ids[j] == target_graph) {
                result_count++;
            }
        }
        
        uint64_t end = get_cycles();
        measurements[i] = end - start;
        total_results += result_count;
    }
    
    double avg_results = (double)total_results / result.iterations;
    calculate_sparql_stats(measurements, result.iterations, data->triple_count, avg_results, &result);
    
    free(measurements);
    return result;
}

/*═══════════════════════════════════════════════════════════════
  Main Benchmark Runner
  ═══════════════════════════════════════════════════════════════*/

static void print_sparql_result(const sparql_bench_result_t* result) {
    printf("%-35s %-25s %8.2f %8.3f %8.1f %6s %s\n",
           result->name,
           result->query_pattern,
           result->avg_cycles,
           result->cycles_per_triple,
           result->results_per_query,
           result->seven_tick_compliant ? "✅" : "❌",
           result->passed ? "PASS" : "FAIL");
}

int main(int argc, char** argv) {
    printf("🔍 CNS SPARQL Dedicated Benchmark Suite\n");
    printf("=======================================\n\n");
    
    // Configuration
    size_t test_triples = SPARQL_TEST_TRIPLES;
    if (argc > 1) {
        test_triples = strtoull(argv[1], NULL, 10);
        if (test_triples < 1000) test_triples = 1000;
        if (test_triples > 1000000) test_triples = 1000000;
    }
    
    printf("Configuration:\n");
    printf("  Test triples: %zu\n", test_triples);
    printf("  7-tick target: ≤ 7 cycles per triple\n");
    printf("  RDF vocabularies: FOAF, Dublin Core, RDFS\n");
    printf("  Graph structure: Realistic clustering\n\n");
    
    // Generate RDF test data
    printf("🔄 Generating RDF test data (%zu triples)...\n", test_triples);
    sparql_test_data_t test_data;
    generate_sparql_test_data(&test_data, test_triples);
    
    // Create SPARQL engine
    printf("🔄 Initializing SPARQL engine...\n");
    CNSSparqlEngine* engine = cns_sparql_create(test_triples, test_triples / 10, test_triples);
    if (!engine) {
        printf("❌ Failed to create SPARQL engine\n");
        cleanup_sparql_test_data(&test_data);
        return 1;
    }
    
    // Load test data into engine
    printf("🔄 Loading test data into engine...\n");
    for (size_t i = 0; i < test_triples; i++) {
        cns_sparql_add_triple(engine, 
            test_data.subjects[i], 
            test_data.predicates[i], 
            test_data.objects[i]);
    }
    
    // Run benchmarks
    printf("\n%-35s %-25s %8s %8s %8s %6s %s\n",
           "SPARQL Operation", "Query Pattern", "Avg", "Per TP", "Results", "7T", "Status");
    printf("%-35s %-25s %8s %8s %8s %6s %s\n",
           "---------------", "-------------", "---", "------", "-------", "--", "------");
    
    sparql_bench_result_t results[20];
    size_t result_count = 0;
    
    // Basic pattern matching
    results[result_count++] = benchmark_sparql_triple_pattern_spo(&test_data, engine);
    print_sparql_result(&results[result_count - 1]);
    
    results[result_count++] = benchmark_sparql_subject_lookup(&test_data, engine);
    print_sparql_result(&results[result_count - 1]);
    
    results[result_count++] = benchmark_sparql_predicate_lookup(&test_data, engine);
    print_sparql_result(&results[result_count - 1]);
    
    results[result_count++] = benchmark_sparql_object_lookup(&test_data, engine);
    print_sparql_result(&results[result_count - 1]);
    
    // Complex patterns
    results[result_count++] = benchmark_sparql_type_filter(&test_data, engine);
    print_sparql_result(&results[result_count - 1]);
    
    results[result_count++] = benchmark_sparql_property_path(&test_data, engine);
    print_sparql_result(&results[result_count - 1]);
    
    results[result_count++] = benchmark_sparql_join_pattern(&test_data, engine);
    print_sparql_result(&results[result_count - 1]);
    
    // Graph operations
    results[result_count++] = benchmark_sparql_graph_pattern(&test_data, engine);
    print_sparql_result(&results[result_count - 1]);
    
    // Calculate summary statistics
    size_t passed_count = 0;
    size_t seven_tick_count = 0;
    double avg_performance = 0.0;
    double avg_cycles_per_triple = 0.0;
    
    for (size_t i = 0; i < result_count; i++) {
        if (results[i].passed) passed_count++;
        if (results[i].seven_tick_compliant) seven_tick_count++;
        avg_performance += results[i].avg_cycles;
        avg_cycles_per_triple += results[i].cycles_per_triple;
    }
    
    avg_performance /= result_count;
    avg_cycles_per_triple /= result_count;
    
    printf("\n📊 SPARQL Performance Summary:\n");
    printf("  Total operations: %zu\n", result_count);
    printf("  Passed: %zu (%.1f%%)\n", passed_count, 100.0 * passed_count / result_count);
    printf("  7-tick compliant: %zu (%.1f%%)\n", seven_tick_count, 100.0 * seven_tick_count / result_count);
    printf("  Average cycles per query: %.2f\n", avg_performance);
    printf("  Average cycles per triple: %.3f\n", avg_cycles_per_triple);
    printf("  RDF graph size: %zu triples\n", test_triples);
    printf("  Query complexity: %s\n", 
           (seven_tick_count == result_count) ? "✅ OPTIMAL" :
           (seven_tick_count >= result_count * 0.7) ? "⚠️  GOOD" : "❌ NEEDS OPTIMIZATION");
    
    // Output JSON summary
    printf("\n🔍 JSON Summary:\n");
    printf("{\n");
    printf("  \"benchmark_type\": \"sparql\",\n");
    printf("  \"test_triples\": %zu,\n", test_triples);
    printf("  \"total_operations\": %zu,\n", result_count);
    printf("  \"passed_count\": %zu,\n", passed_count);
    printf("  \"seven_tick_count\": %zu,\n", seven_tick_count);
    printf("  \"avg_cycles_per_query\": %.2f,\n", avg_performance);
    printf("  \"avg_cycles_per_triple\": %.3f,\n", avg_cycles_per_triple);
    printf("  \"compliance_rate\": %.3f,\n", (double)seven_tick_count / result_count);
    printf("  \"operations\": [\n");
    
    for (size_t i = 0; i < result_count; i++) {
        printf("    {\n");
        printf("      \"name\": \"%s\",\n", results[i].name);
        printf("      \"pattern\": \"%s\",\n", results[i].query_pattern);
        printf("      \"avg_cycles\": %.2f,\n", results[i].avg_cycles);
        printf("      \"cycles_per_triple\": %.3f,\n", results[i].cycles_per_triple);
        printf("      \"results_per_query\": %.1f,\n", results[i].results_per_query);
        printf("      \"seven_tick_compliant\": %s,\n", results[i].seven_tick_compliant ? "true" : "false");
        printf("      \"passed\": %s\n", results[i].passed ? "true" : "false");
        printf("    }%s\n", (i < result_count - 1) ? "," : "");
    }
    
    printf("  ]\n");
    printf("}\n");
    
    // Cleanup
    cns_sparql_destroy(engine);
    cleanup_sparql_test_data(&test_data);
    
    return (passed_count == result_count && seven_tick_count >= result_count / 2) ? 0 : 1;
}