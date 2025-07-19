/*
 * Debug parser for AOT compiler output
 */

#include <stdio.h>
#include <string.h>

int main() {
    const char* test_output = 
"🚀 CNS Production AOT Compiler v2.0\n"
"   Ontologies: docs/ontology\n"
"   SQL: examples/sql\n"
"   Output: build/generated/debug_test\n"
"📖 Processing ontologies...\n"
"    - Loading cns-compilers.ttl\n"
"✅ AOT Compilation Complete\n"
"    - Total time: 0.11ms\n"
"    - Parse time: 0.11ms\n"
"    - Generation time: 0.00ms\n"
"    - Generated 76 ontology rules\n"
"    - Generated 38 SHACL shapes\n"
"    - Generated 8 SQL queries\n"
"    - Efficiency: 690.4 rules/ms\n";
    
    printf("Testing parsing of AOT output...\n");
    
    // Test rules parsing
    char* rules_line = strstr(test_output, "ontology rules");
    if (rules_line) {
        char* p = rules_line - 1;
        while (p > test_output && (*p == ' ' || *p == '\t')) p--;
        while (p > test_output && *p >= '0' && *p <= '9') p--;
        if (p < rules_line) {
            int rules;
            sscanf(p + 1, "%d", &rules);
            printf("✅ Parsed rules: %d\n", rules);
        }
    } else {
        printf("❌ Rules line not found\n");
    }
    
    // Test shapes parsing
    char* shapes_line = strstr(test_output, "SHACL shapes");
    if (shapes_line) {
        char* p = shapes_line - 1;
        while (p > test_output && (*p == ' ' || *p == '\t')) p--;
        while (p > test_output && *p >= '0' && *p <= '9') p--;
        if (p < shapes_line) {
            int shapes;
            sscanf(p + 1, "%d", &shapes);
            printf("✅ Parsed shapes: %d\n", shapes);
        }
    } else {
        printf("❌ Shapes line not found\n");
    }
    
    // Test queries parsing
    char* queries_line = strstr(test_output, "SQL queries");
    if (queries_line) {
        char* p = queries_line - 1;
        while (p > test_output && (*p == ' ' || *p == '\t')) p--;
        while (p > test_output && *p >= '0' && *p <= '9') p--;
        if (p < queries_line) {
            int queries;
            sscanf(p + 1, "%d", &queries);
            printf("✅ Parsed queries: %d\n", queries);
        }
    } else {
        printf("❌ Queries line not found\n");
    }
    
    // Test efficiency parsing
    char* efficiency_line = strstr(test_output, "Efficiency: ");
    if (efficiency_line) {
        double efficiency;
        sscanf(efficiency_line, "Efficiency: %lf rules/ms", &efficiency);
        printf("✅ Parsed efficiency: %.1f rules/ms\n", efficiency);
    } else {
        printf("❌ Efficiency line not found\n");
    }
    
    return 0;
}