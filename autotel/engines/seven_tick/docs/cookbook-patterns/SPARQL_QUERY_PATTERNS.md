# SPARQL Query Patterns Cookbook

## Overview

This cookbook provides practical patterns and examples for using the 7T SPARQL engine to query knowledge graphs with high performance. The SPARQL engine achieves sub-microsecond performance for core query operations.

## Table of Contents

1. [Basic Query Patterns](#basic-query-patterns)
2. [Pattern Matching](#pattern-matching)
3. [Join Patterns](#join-patterns)
4. [Aggregation Patterns](#aggregation-patterns)
5. [Filter Patterns](#filter-patterns)
6. [Performance Optimization Patterns](#performance-optimization-patterns)
7. [Integration Patterns](#integration-patterns)

## Basic Query Patterns

### Pattern 1: Simple Triple Pattern

**Use Case**: Find all subjects with a given predicate and object.

```sparql
SELECT ?s WHERE { ?s rdf:type ex:Person }
```

**C Implementation:**
```c
#include "../runtime/src/seven_t_runtime.h"

void find_all_persons(EngineState* engine) {
    uint32_t type_id = s7t_intern_string(engine, "rdf:type");
    uint32_t person_id = s7t_intern_string(engine, "ex:Person");
    
    size_t count;
    uint32_t* subjects = s7t_get_subjects(engine, type_id, person_id, &count);
    
    printf("Persons found: %zu\n", count);
    for (size_t i = 0; i < count; i++) {
        printf("  - Node ID: %u\n", subjects[i]);
    }
}
```

### Pattern 2: Property Value Lookup

**Use Case**: Get all values for a property of a subject.

```sparql
SELECT ?email WHERE { ex:john ex:email ?email }
```

**C Implementation:**
```c
void get_emails_for_john(EngineState* engine) {
    uint32_t john_id = s7t_intern_string(engine, "ex:john");
    uint32_t email_id = s7t_intern_string(engine, "ex:email");
    
    size_t count;
    uint32_t* emails = s7t_get_objects(engine, email_id, john_id, &count);
    
    printf("Emails for ex:john: %zu\n", count);
    for (size_t i = 0; i < count; i++) {
        printf("  - Email ID: %u\n", emails[i]);
    }
}
```

## Pattern Matching

### Pattern 3: Multi-Triple Pattern

**Use Case**: Find all subjects with multiple properties.

```sparql
SELECT ?s WHERE {
  ?s rdf:type ex:Person .
  ?s ex:email ?email .
}
```

**C Implementation:**
```c
void find_persons_with_email(EngineState* engine) {
    uint32_t type_id = s7t_intern_string(engine, "rdf:type");
    uint32_t person_id = s7t_intern_string(engine, "ex:Person");
    uint32_t email_id = s7t_intern_string(engine, "ex:email");
    
    size_t count;
    uint32_t* persons = s7t_get_subjects(engine, type_id, person_id, &count);
    
    printf("Persons with email:\n");
    for (size_t i = 0; i < count; i++) {
        size_t email_count;
        uint32_t* emails = s7t_get_objects(engine, email_id, persons[i], &email_count);
        if (email_count > 0) {
            printf("  - Node ID: %u\n", persons[i]);
        }
    }
}
```

### Pattern 4: Optional Pattern

**Use Case**: Get values if present, but don't require them.

```sparql
SELECT ?s ?email WHERE {
  ?s rdf:type ex:Person .
  OPTIONAL { ?s ex:email ?email }
}
```

**C Implementation:**
```c
void find_persons_with_optional_email(EngineState* engine) {
    uint32_t type_id = s7t_intern_string(engine, "rdf:type");
    uint32_t person_id = s7t_intern_string(engine, "ex:Person");
    uint32_t email_id = s7t_intern_string(engine, "ex:email");
    
    size_t count;
    uint32_t* persons = s7t_get_subjects(engine, type_id, person_id, &count);
    
    for (size_t i = 0; i < count; i++) {
        size_t email_count;
        uint32_t* emails = s7t_get_objects(engine, email_id, persons[i], &email_count);
        printf("Person %u: ", persons[i]);
        if (email_count > 0) {
            printf("Email ID: %u\n", emails[0]);
        } else {
            printf("No email\n");
        }
    }
}
```

## Join Patterns

### Pattern 5: Join on Shared Property

**Use Case**: Find pairs of nodes sharing a property value.

```sparql
SELECT ?a ?b WHERE {
  ?a ex:email ?email .
  ?b ex:email ?email .
  FILTER(?a != ?b)
}
```

**C Implementation:**
```c
void find_shared_email_pairs(EngineState* engine) {
    uint32_t email_id = s7t_intern_string(engine, "ex:email");
    
    // For demo, assume a small set of emails
    uint32_t emails[] = {
        s7t_intern_string(engine, "john@example.com"),
        s7t_intern_string(engine, "jane@example.com")
    };
    size_t email_count = 2;
    
    for (size_t i = 0; i < email_count; i++) {
        size_t subj_count;
        uint32_t* subjects = s7t_get_subjects(engine, email_id, emails[i], &subj_count);
        for (size_t j = 0; j < subj_count; j++) {
            for (size_t k = 0; k < subj_count; k++) {
                if (j != k) {
                    printf("Pair: %u, %u (email: %u)\n", subjects[j], subjects[k], emails[i]);
                }
            }
        }
    }
}
```

## Aggregation Patterns

### Pattern 6: Count Aggregation

**Use Case**: Count the number of nodes matching a pattern.

```sparql
SELECT (COUNT(?s) AS ?count) WHERE { ?s rdf:type ex:Person }
```

**C Implementation:**
```c
void count_persons(EngineState* engine) {
    uint32_t type_id = s7t_intern_string(engine, "rdf:type");
    uint32_t person_id = s7t_intern_string(engine, "ex:Person");
    
    size_t count;
    s7t_get_subjects(engine, type_id, person_id, &count);
    printf("Number of persons: %zu\n", count);
}
```

### Pattern 7: Group By Aggregation

**Use Case**: Group nodes by a property value.

```sparql
SELECT ?role (COUNT(?s) AS ?count) WHERE {
  ?s ex:role ?role
} GROUP BY ?role
```

**C Implementation:**
```c
void group_by_role(EngineState* engine, uint32_t* roles, size_t role_count) {
    uint32_t role_id = s7t_intern_string(engine, "ex:role");
    for (size_t i = 0; i < role_count; i++) {
        size_t count;
        uint32_t* subjects = s7t_get_subjects(engine, role_id, roles[i], &count);
        printf("Role %u: %zu members\n", roles[i], count);
    }
}
```

## Filter Patterns

### Pattern 8: Value Filter

**Use Case**: Filter results by value.

```sparql
SELECT ?s WHERE {
  ?s ex:age ?age .
  FILTER(?age > 30)
}
```

**C Implementation:**
```c
void filter_by_age(EngineState* engine, uint32_t* subjects, size_t subject_count, uint32_t age_id) {
    for (size_t i = 0; i < subject_count; i++) {
        size_t count;
        uint32_t* ages = s7t_get_objects(engine, age_id, subjects[i], &count);
        if (count > 0 && ages[0] > 30) {
            printf("Subject %u is over 30 (age: %u)\n", subjects[i], ages[0]);
        }
    }
}
```

## Performance Optimization Patterns

### Pattern 9: Batch Query Execution

**Use Case**: Execute multiple queries efficiently.

```c
typedef struct {
    const char* query;
    const char* name;
} QueryDefinition;

void batch_query_execution(EngineState* engine, QueryDefinition* queries, size_t query_count) {
    for (size_t i = 0; i < query_count; i++) {
        printf("Executing query: %s\n", queries[i].name);
        // Parse and execute queries[i].query (pseudo-code)
        // ...
    }
}
```

### Pattern 10: Query Result Caching

**Use Case**: Cache results for repeated queries.

```c
typedef struct {
    const char* query;
    uint32_t* result;
    size_t result_count;
    int valid;
} QueryCacheEntry;

typedef struct {
    QueryCacheEntry* entries;
    size_t capacity;
    size_t count;
} QueryCache;

QueryCache* create_query_cache(size_t capacity) {
    QueryCache* cache = malloc(sizeof(QueryCache));
    cache->entries = calloc(capacity, sizeof(QueryCacheEntry));
    cache->capacity = capacity;
    cache->count = 0;
    return cache;
}

void destroy_query_cache(QueryCache* cache) {
    for (size_t i = 0; i < cache->count; i++) {
        free(cache->entries[i].result);
    }
    free(cache->entries);
    free(cache);
}
```

## Integration Patterns

### Pattern 11: SPARQL with SHACL Integration

**Use Case**: Use SPARQL queries to select nodes for SHACL validation.

```c
void validate_sparql_selected_nodes(EngineState* engine) {
    // Find all Person nodes
    uint32_t type_id = s7t_intern_string(engine, "rdf:type");
    uint32_t person_id = s7t_intern_string(engine, "ex:Person");
    size_t count;
    uint32_t* persons = s7t_get_subjects(engine, type_id, person_id, &count);
    
    // Validate each person node (using SHACL patterns)
    for (size_t i = 0; i < count; i++) {
        // ... call SHACL validation logic ...
    }
}
```

### Pattern 12: SPARQL with CJinja Integration

**Use Case**: Render query results using CJinja templates.

```c
#include "../compiler/src/cjinja.h"

void render_query_results_with_cjinja(EngineState* engine, uint32_t* results, size_t result_count) {
    CJinjaEngine* jinja_engine = cjinja_create("./templates");
    CJinjaContext* ctx = cjinja_create_context();
    
    // Set query results as array
    cjinja_set_array(ctx, "results", (char**)results, result_count);
    cjinja_set_var(ctx, "result_count", "3");
    
    // Template for rendering
    const char* template = 
        "Query Results\n"
        "============\n"
        "Total: {{result_count}}\n"
        "{% for result in results %}"
        "  - {{result}}\n"
        "{% endfor %}";
    
    char* report = cjinja_render_with_loops(template, ctx);
    printf("%s\n", report);
    
    free(report);
    cjinja_destroy_context(ctx);
    cjinja_destroy(jinja_engine);
}
```

## Performance Best Practices

### 1. String Interning
Intern all query terms once and reuse the IDs for best performance.

### 2. Batch Querying
Group queries to minimize overhead and maximize cache locality.

### 3. Result Caching
Cache results for repeated queries to avoid recomputation.

### 4. Memory Management
Always free allocated memory for query results and caches.

## Conclusion

The SPARQL query patterns in this cookbook demonstrate how to achieve sub-microsecond performance for core query operations while providing comprehensive query capabilities. Key takeaways:

1. **Pattern matching**: Efficient triple and multi-triple queries
2. **Joins and aggregations**: High-performance join and group-by patterns
3. **Filtering**: Fast value-based filtering
4. **Batch and cached queries**: Efficient repeated and grouped queries
5. **Integration**: Seamless integration with SHACL validation and CJinja rendering

These patterns provide a solid foundation for building high-performance query systems on the 7T engine. 