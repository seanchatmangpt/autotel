#include "../compiler/src/cjinja.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Demo 3: SPARQL Result Formatting with 7T Engine Integration (Simplified)
// Use Case: Formatting SPARQL query results using both 7-tick and 49-tick paths

// Simplified SPARQL simulation functions
typedef struct {
    int subject;
    int predicate;
    int object;
} Triple;

typedef struct {
    Triple *triples;
    size_t count;
    size_t capacity;
} S7TEngine;

S7TEngine* s7t_create(size_t max_triples, size_t max_subjects, size_t max_objects) {
    S7TEngine *engine = malloc(sizeof(S7TEngine));
    engine->triples = malloc(max_triples * sizeof(Triple));
    engine->count = 0;
    engine->capacity = max_triples;
    return engine;
}

void s7t_add_triple(S7TEngine *engine, int s, int p, int o) {
    if (engine->count < engine->capacity) {
        engine->triples[engine->count].subject = s;
        engine->triples[engine->count].predicate = p;
        engine->triples[engine->count].object = o;
        engine->count++;
    }
}

int s7t_ask_pattern(S7TEngine *engine, int s, int p, int o) {
    for (size_t i = 0; i < engine->count; i++) {
        if (engine->triples[i].subject == s && 
            engine->triples[i].predicate == p && 
            engine->triples[i].object == o) {
            return 1;
        }
    }
    return 0;
}

void s7t_destroy(S7TEngine *engine) {
    free(engine->triples);
    free(engine);
}

int main() {
    printf("Demo 3: SPARQL Result Formatting (7T Engine Integration)\n");
    printf("=======================================================\n\n");
    
    // Create SPARQL engine
    S7TEngine *sparql = s7t_create(10000, 100, 10000);
    if (!sparql) {
        printf("Failed to create SPARQL engine\n");
        return 1;
    }
    
    // Create CJinja engine and context
    CJinjaEngine *cjinja = cjinja_create("./templates");
    CJinjaContext *ctx = cjinja_create_context();
    
    if (!cjinja || !ctx) {
        printf("Failed to create CJinja engine or context\n");
        return 1;
    }
    
    printf("Setting up sample RDF data...\n");
    
    // Add sample RDF data (subject, predicate, object)
    s7t_add_triple(sparql, 1, 1, 2);  // (Alice, knows, Bob)
    s7t_add_triple(sparql, 1, 1, 3);  // (Alice, knows, Charlie)
    s7t_add_triple(sparql, 1, 2, 4);  // (Alice, worksAt, TechCorp)
    s7t_add_triple(sparql, 2, 1, 3);  // (Bob, knows, Charlie)
    s7t_add_triple(sparql, 2, 2, 5);  // (Bob, worksAt, StartupInc)
    s7t_add_triple(sparql, 3, 2, 4);  // (Charlie, worksAt, TechCorp)
    s7t_add_triple(sparql, 3, 3, 6);  // (Charlie, hasSkill, Programming)
    s7t_add_triple(sparql, 1, 3, 7);  // (Alice, hasSkill, Design)
    s7t_add_triple(sparql, 2, 3, 8);  // (Bob, hasSkill, Marketing)
    
    // Set up entity names for display
    cjinja_set_var(ctx, "entity_1", "Alice");
    cjinja_set_var(ctx, "entity_2", "Bob");
    cjinja_set_var(ctx, "entity_3", "Charlie");
    cjinja_set_var(ctx, "entity_4", "TechCorp");
    cjinja_set_var(ctx, "entity_5", "StartupInc");
    cjinja_set_var(ctx, "entity_6", "Programming");
    cjinja_set_var(ctx, "entity_7", "Design");
    cjinja_set_var(ctx, "entity_8", "Marketing");
    
    cjinja_set_var(ctx, "pred_1", "knows");
    cjinja_set_var(ctx, "pred_2", "worksAt");
    cjinja_set_var(ctx, "pred_3", "hasSkill");
    
    printf("Executing SPARQL queries and formatting results...\n\n");
    
    // 1. Simple SPARQL Query - Using 7-tick path for performance
    printf("1. Simple SPARQL Query (7-tick path):\n");
    printf("=====================================\n");
    
    // Query: Who does Alice know?
    int alice_knows_bob = s7t_ask_pattern(sparql, 1, 1, 2);
    int alice_knows_charlie = s7t_ask_pattern(sparql, 1, 1, 3);
    
    const char *simple_template = "Alice knows: {{result}}";
    
    if (alice_knows_bob && alice_knows_charlie) {
        cjinja_set_var(ctx, "result", "Bob and Charlie");
    } else if (alice_knows_bob) {
        cjinja_set_var(ctx, "result", "Bob");
    } else if (alice_knows_charlie) {
        cjinja_set_var(ctx, "result", "Charlie");
    } else {
        cjinja_set_var(ctx, "result", "No one");
    }
    
    char *simple_result = cjinja_render_string_7tick(simple_template, ctx);
    printf("Query: Who does Alice know?\n");
    printf("Result: %s\n", simple_result);
    free(simple_result);
    
    // 2. Complex SPARQL Query - Using 49-tick path for advanced formatting
    printf("\n2. Complex SPARQL Query (49-tick path):\n");
    printf("=======================================\n");
    
    // Query: Who works at TechCorp and what are their skills?
    int alice_works_techcorp = s7t_ask_pattern(sparql, 1, 2, 4);
    int charlie_works_techcorp = s7t_ask_pattern(sparql, 3, 2, 4);
    int alice_has_design = s7t_ask_pattern(sparql, 1, 3, 7);
    int charlie_has_programming = s7t_ask_pattern(sparql, 3, 3, 6);
    
    // Set up arrays for complex template
    char *techcorp_employees[] = {"Alice", "Charlie"};
    cjinja_set_array(ctx, "employees", techcorp_employees, 2);
    
    char *alice_skills[] = {"Design"};
    cjinja_set_array(ctx, "alice_skills", alice_skills, 1);
    
    char *charlie_skills[] = {"Programming"};
    cjinja_set_array(ctx, "charlie_skills", charlie_skills, 1);
    
    const char *complex_template = 
        "TechCorp Employee Analysis\n"
        "==========================\n"
        "Company: {{company_name | upper}}\n"
        "Employee Count: {{employees | length}}\n\n"
        "Employees and Skills:\n"
        "{% for employee in employees %}\n"
        "  {{employee | capitalize}}:\n"
        "  {% if employee == 'Alice' %}\n"
        "    Skills: {{alice_skills | join(', ')}}\n"
        "  {% elif employee == 'Charlie' %}\n"
        "    Skills: {{charlie_skills | join(', ')}}\n"
        "  {% endif %}\n"
        "{% endfor %}\n\n"
        "Summary: {{employees | length}} employees with diverse skills.";
    
    cjinja_set_var(ctx, "company_name", "TechCorp");
    
    char *complex_result = cjinja_render_with_loops(complex_template, ctx);
    printf("Query: Who works at TechCorp and what are their skills?\n");
    printf("Result:\n%s\n", complex_result);
    free(complex_result);
    
    // 3. Batch SPARQL Queries - Using batch rendering
    printf("\n3. Batch SPARQL Queries (Batch rendering):\n");
    printf("==========================================\n");
    
    // Execute multiple SPARQL queries
    int queries[] = {
        s7t_ask_pattern(sparql, 1, 1, 2),  // Alice knows Bob
        s7t_ask_pattern(sparql, 2, 2, 5),  // Bob works at StartupInc
        s7t_ask_pattern(sparql, 3, 3, 6),  // Charlie has Programming skill
        s7t_ask_pattern(sparql, 1, 3, 7),  // Alice has Design skill
        s7t_ask_pattern(sparql, 2, 3, 8)   // Bob has Marketing skill
    };
    
    const char *query_names[] = {
        "Alice knows Bob",
        "Bob works at StartupInc", 
        "Charlie has Programming skill",
        "Alice has Design skill",
        "Bob has Marketing skill"
    };
    
    const char *batch_templates[] = {
        "Query: {{query_name}} | Result: {{result}}",
        "{{query_name}}: {{result}}",
        "✓ {{query_name}} → {{result}}",
        "{{query_name}} = {{result}}",
        "{{query_name}}: {{result}}"
    };
    
    CJinjaBatchRender *batch = cjinja_create_batch_render(5);
    
    for (int i = 0; i < 5; i++) {
        batch->templates[i] = batch_templates[i];
        cjinja_set_var(ctx, "query_name", query_names[i]);
        cjinja_set_var(ctx, "result", queries[i] ? "True" : "False");
    }
    
    int batch_result = cjinja_render_batch(cjinja, batch, ctx);
    if (batch_result == 0) {
        printf("Batch query results:\n");
        for (int i = 0; i < 5; i++) {
            printf("  %s\n", batch->results[i]);
        }
    }
    
    // 4. Performance comparison: 7-tick vs 49-tick for SPARQL results
    printf("\n4. Performance Comparison (7-tick vs 49-tick):\n");
    printf("==============================================\n");
    
    const char *simple_sparql_template = "{{subject}} {{predicate}} {{object}}";
    const char *complex_sparql_template = 
        "{{subject | capitalize}} {{predicate | lower}} {{object | capitalize}} "
        "{% if is_valid %}✓{% else %}✗{% endif %}";
    
    // Set up test data
    cjinja_set_var(ctx, "subject", "Alice");
    cjinja_set_var(ctx, "predicate", "knows");
    cjinja_set_var(ctx, "object", "Bob");
    cjinja_set_bool(ctx, "is_valid", 1);
    
    // Test 7-tick performance
    clock_t start = clock();
    for (int i = 0; i < 1000; i++) {
        char *result = cjinja_render_string_7tick(simple_sparql_template, ctx);
        free(result);
    }
    clock_t end = clock();
    double cpu_time_7tick = ((double) (end - start)) / CLOCKS_PER_SEC;
    
    // Test 49-tick performance
    start = clock();
    for (int i = 0; i < 1000; i++) {
        char *result = cjinja_render_with_loops(complex_sparql_template, ctx);
        free(result);
    }
    end = clock();
    double cpu_time_49tick = ((double) (end - start)) / CLOCKS_PER_SEC;
    
    printf("Performance for 1000 SPARQL result renders:\n");
    printf("  7-tick path: %.3f seconds (%.1f μs per render)\n", 
           cpu_time_7tick, (cpu_time_7tick * 1000000.0) / 1000);
    printf("  49-tick path: %.3f seconds (%.1f μs per render)\n", 
           cpu_time_49tick, (cpu_time_49tick * 1000000.0) / 1000);
    printf("  Speedup: %.1fx faster with 7-tick path\n", cpu_time_49tick / cpu_time_7tick);
    
    // Show sample outputs
    char *simple_output = cjinja_render_string_7tick(simple_sparql_template, ctx);
    char *complex_output = cjinja_render_with_loops(complex_sparql_template, ctx);
    
    printf("\nSample outputs:\n");
    printf("  7-tick: %s\n", simple_output);
    printf("  49-tick: %s\n", complex_output);
    
    free(simple_output);
    free(complex_output);
    
    // Cleanup
    cjinja_destroy_batch_render(batch);
    cjinja_destroy_context(ctx);
    cjinja_destroy_engine(cjinja);
    s7t_destroy(sparql);
    
    printf("\nDemo 3 completed! SPARQL integration with both 7-tick and 49-tick paths.\n");
    return 0;
}