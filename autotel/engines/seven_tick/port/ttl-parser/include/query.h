/**
 * @file query.h
 * @brief Simple triple query engine for TTL/Turtle documents
 * @author CNS Seven-Tick Engine
 * @date 2024
 * 
 * This file defines a basic query interface for pattern matching against
 * parsed TTL documents. Implements 80/20 functionality focusing on the most
 * common query patterns.
 */

#ifndef TTL_QUERY_H
#define TTL_QUERY_H

#include "ast.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct ttl_query_engine ttl_query_engine_t;
typedef struct ttl_query_pattern ttl_query_pattern_t;
typedef struct ttl_query_result ttl_query_result_t;
typedef struct ttl_query_binding ttl_query_binding_t;
typedef struct ttl_query_filter ttl_query_filter_t;

/**
 * Query pattern element types
 */
typedef enum {
    TTL_PATTERN_VARIABLE,     /* ?var - variable to bind */
    TTL_PATTERN_FIXED,        /* fixed value - exact match */
    TTL_PATTERN_WILDCARD      /* * - matches anything */
} ttl_pattern_element_type_t;

/**
 * Query filter types (for Priority 2 features)
 */
typedef enum {
    TTL_FILTER_STRING_LITERAL,    /* String literal exact match */
    TTL_FILTER_STRING_REGEX,      /* String regex match */
    TTL_FILTER_NUMERIC_RANGE,     /* Numeric range match */
    TTL_FILTER_LANGUAGE_TAG,      /* Language tag match */
    TTL_FILTER_DATATYPE          /* Datatype match */
} ttl_query_filter_type_t;

/**
 * Pattern element for subject, predicate, or object position
 */
typedef struct {
    ttl_pattern_element_type_t type;
    union {
        char *variable_name;      /* For VARIABLE type */
        char *fixed_value;        /* For FIXED type */
    } data;
} ttl_pattern_element_t;

/**
 * Triple pattern structure
 * Represents patterns like: ?s rdf:type foaf:Person
 */
struct ttl_query_pattern {
    ttl_pattern_element_t subject;
    ttl_pattern_element_t predicate;
    ttl_pattern_element_t object;
    
    /* Optional filters */
    ttl_query_filter_t *filters;
    size_t filter_count;
};

/**
 * Variable binding in query results
 */
struct ttl_query_binding {
    char *variable_name;
    ttl_ast_node_t *value;    /* Bound AST node */
    char *string_value;       /* String representation */
};

/**
 * Query result set
 */
struct ttl_query_result {
    ttl_query_binding_t *bindings;
    size_t binding_count;
    size_t binding_capacity;
    
    /* Result iteration */
    size_t current_row;
    size_t row_count;
    ttl_query_binding_t **rows;  /* Array of binding arrays */
};

/**
 * Query filter structure
 */
struct ttl_query_filter {
    ttl_query_filter_type_t type;
    char *target_variable;    /* Variable to filter */
    
    union {
        struct {
            char *value;
        } string_literal;
        
        struct {
            char *pattern;
            bool case_insensitive;
        } string_regex;
        
        struct {
            double min_value;
            double max_value;
            bool include_min;
            bool include_max;
        } numeric_range;
        
        struct {
            char *language;
        } language_tag;
        
        struct {
            char *datatype_iri;
        } datatype;
    } data;
};

/**
 * Query engine (main interface)
 */
struct ttl_query_engine {
    ttl_ast_node_t *document;     /* Document to query */
    ttl_ast_context_t *context;   /* AST context */
    
    /* Cached data for performance */
    ttl_ast_node_t **triples;     /* All triples in document */
    size_t triple_count;
    
    /* Statistics */
    struct {
        size_t queries_executed;
        size_t patterns_matched;
        size_t total_results;
    } stats;
};

/**
 * Create query engine from parsed document
 * @param document Parsed TTL document (AST root)
 * @param context AST context (for memory management)
 * @return New query engine instance
 */
ttl_query_engine_t* ttl_query_engine_create(ttl_ast_node_t *document, ttl_ast_context_t *context);

/**
 * Destroy query engine
 * @param engine Query engine to destroy
 */
void ttl_query_engine_destroy(ttl_query_engine_t *engine);

/**
 * Create simple triple pattern
 * @param subject_pattern Subject pattern (e.g., "?s" or "<http://example.org/person1>")
 * @param predicate_pattern Predicate pattern (e.g., "rdf:type" or "?p")
 * @param object_pattern Object pattern (e.g., "foaf:Person" or "?o")
 * @return New pattern instance
 */
ttl_query_pattern_t* ttl_query_pattern_create(const char *subject_pattern,
                                              const char *predicate_pattern,
                                              const char *object_pattern);

/**
 * Destroy query pattern
 * @param pattern Pattern to destroy
 */
void ttl_query_pattern_destroy(ttl_query_pattern_t *pattern);

/**
 * Execute query pattern against document
 * @param engine Query engine
 * @param pattern Query pattern
 * @return Query results (NULL on error)
 */
ttl_query_result_t* ttl_query_execute(ttl_query_engine_t *engine, ttl_query_pattern_t *pattern);

/**
 * Execute simple query from string
 * Convenience function for basic queries like: "?s rdf:type foaf:Person"
 * @param engine Query engine
 * @param query_string Simple query string
 * @return Query results (NULL on error)
 */
ttl_query_result_t* ttl_query_execute_simple(ttl_query_engine_t *engine, const char *query_string);

/**
 * Get number of result rows
 * @param result Query result
 * @return Number of rows
 */
size_t ttl_query_result_count(const ttl_query_result_t *result);

/**
 * Get variable names in result
 * @param result Query result
 * @param variable_names Output array of variable names
 * @param count Output count of variables
 */
void ttl_query_result_get_variables(const ttl_query_result_t *result, 
                                   char ***variable_names, 
                                   size_t *count);

/**
 * Get binding value for variable in current row
 * @param result Query result
 * @param variable_name Variable name (e.g., "s", "p", "o")
 * @return Binding value (NULL if not found)
 */
const ttl_query_binding_t* ttl_query_result_get_binding(const ttl_query_result_t *result,
                                                       const char *variable_name);

/**
 * Move to next result row
 * @param result Query result
 * @return True if moved to valid row, false if no more rows
 */
bool ttl_query_result_next(ttl_query_result_t *result);

/**
 * Reset result iterator to first row
 * @param result Query result
 */
void ttl_query_result_reset(ttl_query_result_t *result);

/**
 * Destroy query result
 * @param result Result to destroy
 */
void ttl_query_result_destroy(ttl_query_result_t *result);

/**
 * Add string literal filter to pattern
 * @param pattern Query pattern
 * @param variable Variable to filter
 * @param value String value to match
 */
void ttl_query_pattern_add_string_filter(ttl_query_pattern_t *pattern,
                                        const char *variable,
                                        const char *value);

/**
 * Add regex filter to pattern  
 * @param pattern Query pattern
 * @param variable Variable to filter
 * @param regex Regular expression pattern
 * @param case_insensitive Case insensitive matching
 */
void ttl_query_pattern_add_regex_filter(ttl_query_pattern_t *pattern,
                                       const char *variable,
                                       const char *regex,
                                       bool case_insensitive);

/**
 * Add numeric range filter to pattern
 * @param pattern Query pattern
 * @param variable Variable to filter
 * @param min_value Minimum value (inclusive)
 * @param max_value Maximum value (inclusive)
 */
void ttl_query_pattern_add_numeric_filter(ttl_query_pattern_t *pattern,
                                         const char *variable,
                                         double min_value,
                                         double max_value);

/**
 * Utility functions for common query patterns
 */

/**
 * Find all triples with specific predicate
 * @param engine Query engine
 * @param predicate Predicate IRI or prefixed name
 * @return Query results
 */
ttl_query_result_t* ttl_query_find_by_predicate(ttl_query_engine_t *engine, const char *predicate);

/**
 * Find all triples with specific subject
 * @param engine Query engine
 * @param subject Subject IRI or prefixed name
 * @return Query results
 */
ttl_query_result_t* ttl_query_find_by_subject(ttl_query_engine_t *engine, const char *subject);

/**
 * Find all instances of a specific type (rdf:type queries)
 * @param engine Query engine
 * @param type_iri Type IRI or prefixed name
 * @return Query results
 */
ttl_query_result_t* ttl_query_find_instances(ttl_query_engine_t *engine, const char *type_iri);

/**
 * Get query engine statistics
 * @param engine Query engine
 * @param queries_executed Output for number of queries executed
 * @param patterns_matched Output for number of patterns matched
 * @param total_results Output for total results returned
 */
void ttl_query_engine_get_stats(const ttl_query_engine_t *engine,
                               size_t *queries_executed,
                               size_t *patterns_matched,
                               size_t *total_results);

/**
 * Print query result in table format
 * @param result Query result
 * @param output Output file handle
 */
void ttl_query_result_print(const ttl_query_result_t *result, FILE *output);

/**
 * Print query result in JSON format
 * @param result Query result
 * @param output Output file handle
 */
void ttl_query_result_print_json(const ttl_query_result_t *result, FILE *output);

#ifdef __cplusplus
}
#endif

#endif /* TTL_QUERY_H */