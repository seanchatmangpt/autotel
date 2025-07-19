#ifndef SPARQL_SIMPLE_QUERIES_H
#define SPARQL_SIMPLE_QUERIES_H

#include "cns/engines/sparql.h"

// Common RDF vocabulary IDs
#define RDF_TYPE 1
#define RDFS_LABEL 2
#define FOAF_NAME 3
#define FOAF_KNOWS 4
#define DC_TITLE 5
#define DC_CREATOR 6

// Class IDs
#define PERSON_CLASS 100
#define DOCUMENT_CLASS 101
#define ORGANIZATION_CLASS 102
#define CUSTOMER_CLASS 103

#define S7T_SQL_MAX_ROWS 10000

// Result structure
typedef struct {
    uint32_t subject_id;
    uint32_t predicate_id;
    uint32_t object_id;
    float value;
} QueryResult;

// Kernel functions (from sparql_kernels.c)
int s7t_scan_by_type(CNSSparqlEngine* engine, uint32_t type_id, uint32_t* results, int max_results);
int s7t_scan_by_predicate(CNSSparqlEngine* engine, uint32_t pred_id, uint32_t* results, int max_results);
int s7t_simd_filter_gt_f32(float* values, int count, float threshold, uint32_t* results);
int s7t_hash_join(CNSSparqlEngine* engine, uint32_t* left, int left_count, uint32_t* right, int right_count, uint32_t* results);
void s7t_project_results(CNSSparqlEngine* engine, uint32_t* ids, int count, QueryResult* results);

// Simple compiled query: Find all persons
static inline int compiled_type_query_person(
    CNSSparqlEngine* engine,
    QueryResult* results,
    int max_results
) {
    uint32_t reg0_ids[S7T_SQL_MAX_ROWS];
    
    // Single operation: scan by type
    int count = s7t_scan_by_type(engine, PERSON_CLASS, reg0_ids, S7T_SQL_MAX_ROWS);
    
    // Project results
    int final_count = (count < max_results) ? count : max_results;
    s7t_project_results(engine, reg0_ids, final_count, results);
    
    return final_count;
}

// Simple compiled query: Find all documents
static inline int compiled_type_query_document(
    CNSSparqlEngine* engine,
    QueryResult* results,
    int max_results
) {
    uint32_t reg0_ids[S7T_SQL_MAX_ROWS];
    
    // Single operation: scan by type
    int count = s7t_scan_by_type(engine, DOCUMENT_CLASS, reg0_ids, S7T_SQL_MAX_ROWS);
    
    // Project results
    int final_count = (count < max_results) ? count : max_results;
    s7t_project_results(engine, reg0_ids, final_count, results);
    
    return final_count;
}

// Simple compiled query: Find all with foaf:name
static inline int compiled_predicate_scan_foaf_name(
    CNSSparqlEngine* engine,
    QueryResult* results,
    int max_results
) {
    uint32_t reg0_ids[S7T_SQL_MAX_ROWS];
    
    // Single operation: scan by predicate
    int count = s7t_scan_by_predicate(engine, FOAF_NAME, reg0_ids, S7T_SQL_MAX_ROWS);
    
    // Project results
    int final_count = (count < max_results) ? count : max_results;
    s7t_project_results(engine, reg0_ids, final_count, results);
    
    return final_count;
}

#endif // SPARQL_SIMPLE_QUERIES_H