/*
 * SPARQL 80/20 Test - Quick test to verify functionality
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

#include "cns/engines/sparql.h"

// Basic vocabulary IDs (80/20 subset)
#define RDF_TYPE 1
#define FOAF_NAME 2
#define FOAF_KNOWS 3
#define DC_CREATOR 4
#define DC_TITLE 5

#define PERSON_CLASS 100
#define DOCUMENT_CLASS 101
#define CUSTOMER_CLASS 102

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

void setup_test_data(CNSSparqlEngine* engine) {
    printf("🔄 Setting up test data...\n");
    
    // Add a small number of triples for testing
    for (int i = 1000; i < 1010; i++) {
        cns_sparql_add_triple(engine, i, RDF_TYPE, PERSON_CLASS);
        cns_sparql_add_triple(engine, i, FOAF_NAME, 5000 + i);
    }
    
    for (int i = 2000; i < 2005; i++) {
        cns_sparql_add_triple(engine, i, RDF_TYPE, DOCUMENT_CLASS);
        cns_sparql_add_triple(engine, i, DC_TITLE, 6000 + i);
    }
    
    printf("✅ Test data loaded\n");
}

// Simple kernel implementation
int simple_scan_by_type(CNSSparqlEngine* engine, uint32_t type_id, uint32_t* results, int max_results) {
    int count = 0;
    
    // Simple scan - check reasonable range
    for (size_t i = 0; i < 10000 && count < max_results; i++) {
        if (cns_sparql_ask_pattern(engine, i, RDF_TYPE, type_id)) {
            results[count++] = i;
        }
    }
    
    return count;
}

void test_basic_query(CNSSparqlEngine* engine) {
    uint32_t results[100];
    
    printf("\n🧪 Testing basic queries...\n");
    
    // Test 1: Find all persons
    uint64_t start = get_cycles();
    int count = simple_scan_by_type(engine, PERSON_CLASS, results, 100);
    uint64_t end = get_cycles();
    
    printf("Type Query (Person): Found %d results in %llu cycles\n", count, end - start);
    if (count > 0) {
        printf("  First result: %u\n", results[0]);
    }
    
    // Test 2: Find all documents
    start = get_cycles();
    count = simple_scan_by_type(engine, DOCUMENT_CLASS, results, 100);
    end = get_cycles();
    
    printf("Type Query (Document): Found %d results in %llu cycles\n", count, end - start);
    if (count > 0) {
        printf("  First result: %u\n", results[0]);
    }
}

int main() {
    printf("🚀 SPARQL 80/20 Test Program\n");
    printf("============================\n\n");
    
    // Create engine with reasonable size
    CNSSparqlEngine* engine = cns_sparql_create(
        10000,  // max subjects
        200,    // max predicates
        10000   // max objects
    );
    
    if (!engine) {
        printf("❌ Failed to create SPARQL engine\n");
        return 1;
    }
    
    printf("✅ Created SPARQL engine\n");
    printf("  Max subjects: %zu\n", engine->max_subjects);
    printf("  Max predicates: %zu\n", engine->max_predicates);
    printf("  Max objects: %zu\n", engine->max_objects);
    
    // Setup and test
    setup_test_data(engine);
    test_basic_query(engine);
    
    // Cleanup
    cns_sparql_destroy(engine);
    printf("\n✅ Test complete!\n");
    
    return 0;
}