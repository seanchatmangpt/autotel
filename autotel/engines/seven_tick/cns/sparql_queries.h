#ifndef SPARQL_QUERIES_AOT_H
#define SPARQL_QUERIES_AOT_H

#include <string.h>
#include "cns/engines/sparql.h"
#include "ontology_ids.h"
#include "sparql_id_mappings.h"

// Result structure for compiled queries
typedef struct {
    uint32_t subject_id;
    uint32_t predicate_id;
    uint32_t object_id;
    float value;
} QueryResult;

// Low-level kernel functions (to be implemented)
int s7t_scan_by_type(CNSSparqlEngine* engine, uint32_t type_id, uint32_t* results, int max_results);
int s7t_scan_by_predicate(CNSSparqlEngine* engine, uint32_t pred_id, uint32_t* results, int max_results);
int s7t_simd_filter_gt_f32(float* values, int count, float threshold, uint32_t* results);
int s7t_hash_join(CNSSparqlEngine* engine, uint32_t* left, int left_count, uint32_t* right, int right_count, uint32_t* results);
void s7t_project_results(CNSSparqlEngine* engine, uint32_t* ids, int count, QueryResult* results);

// Compiled query functions
/**
 * @brief COMPILED SPARQL QUERY: getHighValueCustomers
 * Estimated cycles: 15.0
 * 7-tick compliant: ❌ NO
 */
static inline int getHighValueCustomers(
    CNSSparqlEngine* engine,
    QueryResult* results,
    int max_results
) {
    // Registers for intermediate results
    uint32_t reg0_ids[S7T_SQL_MAX_ROWS];
    float reg0_values[S7T_SQL_MAX_ROWS];
    uint32_t reg1_ids[S7T_SQL_MAX_ROWS];
    float reg1_values[S7T_SQL_MAX_ROWS];
    uint32_t reg2_ids[S7T_SQL_MAX_ROWS];
    float reg2_values[S7T_SQL_MAX_ROWS];
    uint32_t reg3_ids[S7T_SQL_MAX_ROWS];
    float reg3_values[S7T_SQL_MAX_ROWS];
    uint32_t reg4_ids[S7T_SQL_MAX_ROWS];
    float reg4_values[S7T_SQL_MAX_ROWS];
    uint32_t reg5_ids[S7T_SQL_MAX_ROWS];
    float reg5_values[S7T_SQL_MAX_ROWS];
    uint32_t reg6_ids[S7T_SQL_MAX_ROWS];
    float reg6_values[S7T_SQL_MAX_ROWS];

    // Execute micro-operation tape

    // OP: SCAN_TYPE - Find all subjects of type ID__Customer
    int count0 = s7t_scan_by_type(engine, ID__Customer, 
                                    reg0_ids, S7T_SQL_MAX_ROWS);

    // OP: SCAN_PREDICATE - Find all triples with predicate ID__hasName
    int count1 = s7t_scan_by_predicate(engine, ID__hasName,
                                         reg1_ids, S7T_SQL_MAX_ROWS);

    // OP: SCAN_PREDICATE - Find all triples with predicate ID__hasEmail
    int count2 = s7t_scan_by_predicate(engine, ID__hasEmail,
                                         reg2_ids, S7T_SQL_MAX_ROWS);

    // OP: SCAN_PREDICATE - Find all triples with predicate ID__lifetimeValue
    int count3 = s7t_scan_by_predicate(engine, ID__lifetimeValue,
                                         reg3_ids, S7T_SQL_MAX_ROWS);

    // OP: HASH_JOIN - Join registers 0 and 1
    int count4 = s7t_hash_join(engine, 
                                 reg0_ids, count0,
                                 reg1_ids, count1,
                                 reg4_ids);

    // OP: FILTER_GT - Filter values > 5000.0
    int count5 = s7t_simd_filter_gt_f32(reg4_values, count4, 
                                          5000.0f, reg5_ids);

    // OP: PROJECT - Extract final result columns
    int final_count = (count5 < max_results) ? count5 : max_results;
    s7t_project_results(engine, reg5_ids, final_count, results);

    return final_count;
}

/**
 * @brief COMPILED SPARQL QUERY: findPersonsByName
 * Estimated cycles: 12.5
 * 7-tick compliant: ❌ NO
 */
static inline int findPersonsByName(
    CNSSparqlEngine* engine,
    QueryResult* results,
    int max_results
) {
    // Registers for intermediate results
    uint32_t reg0_ids[S7T_SQL_MAX_ROWS];
    float reg0_values[S7T_SQL_MAX_ROWS];
    uint32_t reg1_ids[S7T_SQL_MAX_ROWS];
    float reg1_values[S7T_SQL_MAX_ROWS];
    uint32_t reg2_ids[S7T_SQL_MAX_ROWS];
    float reg2_values[S7T_SQL_MAX_ROWS];
    uint32_t reg3_ids[S7T_SQL_MAX_ROWS];
    float reg3_values[S7T_SQL_MAX_ROWS];
    uint32_t reg4_ids[S7T_SQL_MAX_ROWS];
    float reg4_values[S7T_SQL_MAX_ROWS];
    uint32_t reg5_ids[S7T_SQL_MAX_ROWS];
    float reg5_values[S7T_SQL_MAX_ROWS];

    // Execute micro-operation tape

    // OP: SCAN_TYPE - Find all subjects of type PERSON_CLASS
    int count0 = s7t_scan_by_type(engine, PERSON_CLASS, 
                                    reg0_ids, S7T_SQL_MAX_ROWS);

    // OP: SCAN_PREDICATE - Find all triples with predicate FOAF_NAME
    int count1 = s7t_scan_by_predicate(engine, FOAF_NAME,
                                         reg1_ids, S7T_SQL_MAX_ROWS);

    // OP: SCAN_PREDICATE - Find all triples with predicate ID_foaf_email
    int count2 = s7t_scan_by_predicate(engine, ID_foaf_email,
                                         reg2_ids, S7T_SQL_MAX_ROWS);

    // OP: HASH_JOIN - Join registers 0 and 1
    int count3 = s7t_hash_join(engine, 
                                 reg0_ids, count0,
                                 reg1_ids, count1,
                                 reg3_ids);

    // OP: PROJECT - Extract final result columns
    int final_count = (count3 < max_results) ? count3 : max_results;
    s7t_project_results(engine, reg4_ids, final_count, results);

    return final_count;
}

/**
 * @brief COMPILED SPARQL QUERY: getDocumentsByCreator
 * Estimated cycles: 11.7
 * 7-tick compliant: ❌ NO
 */
static inline int getDocumentsByCreator(
    CNSSparqlEngine* engine,
    QueryResult* results,
    int max_results
) {
    // Registers for intermediate results
    uint32_t reg0_ids[S7T_SQL_MAX_ROWS];
    float reg0_values[S7T_SQL_MAX_ROWS];
    uint32_t reg1_ids[S7T_SQL_MAX_ROWS];
    float reg1_values[S7T_SQL_MAX_ROWS];
    uint32_t reg2_ids[S7T_SQL_MAX_ROWS];
    float reg2_values[S7T_SQL_MAX_ROWS];
    uint32_t reg3_ids[S7T_SQL_MAX_ROWS];
    float reg3_values[S7T_SQL_MAX_ROWS];
    uint32_t reg4_ids[S7T_SQL_MAX_ROWS];
    float reg4_values[S7T_SQL_MAX_ROWS];

    // Execute micro-operation tape

    // OP: SCAN_TYPE - Find all subjects of type DOCUMENT_CLASS
    int count0 = s7t_scan_by_type(engine, DOCUMENT_CLASS, 
                                    reg0_ids, S7T_SQL_MAX_ROWS);

    // OP: SCAN_PREDICATE - Find all triples with predicate DC_CREATOR
    int count1 = s7t_scan_by_predicate(engine, DC_CREATOR,
                                         reg1_ids, S7T_SQL_MAX_ROWS);

    // OP: SCAN_PREDICATE - Find all triples with predicate DC_TITLE
    int count2 = s7t_scan_by_predicate(engine, DC_TITLE,
                                         reg2_ids, S7T_SQL_MAX_ROWS);

    // OP: HASH_JOIN - Join registers 0 and 1
    int count3 = s7t_hash_join(engine, 
                                 reg0_ids, count0,
                                 reg1_ids, count1,
                                 reg3_ids);

    // OP: PROJECT - Extract final result columns
    int final_count = (count3 < max_results) ? count3 : max_results;
    s7t_project_results(engine, reg3_ids, final_count, results);

    return final_count;
}

/**
 * @brief COMPILED SPARQL QUERY: socialConnections
 * Estimated cycles: 10.7
 * 7-tick compliant: ❌ NO
 */
static inline int socialConnections(
    CNSSparqlEngine* engine,
    QueryResult* results,
    int max_results
) {
    // Registers for intermediate results
    uint32_t reg0_ids[S7T_SQL_MAX_ROWS];
    float reg0_values[S7T_SQL_MAX_ROWS];
    uint32_t reg1_ids[S7T_SQL_MAX_ROWS];
    float reg1_values[S7T_SQL_MAX_ROWS];
    uint32_t reg2_ids[S7T_SQL_MAX_ROWS];
    float reg2_values[S7T_SQL_MAX_ROWS];
    uint32_t reg3_ids[S7T_SQL_MAX_ROWS];
    float reg3_values[S7T_SQL_MAX_ROWS];
    uint32_t reg4_ids[S7T_SQL_MAX_ROWS];
    float reg4_values[S7T_SQL_MAX_ROWS];

    // Execute micro-operation tape

    // OP: SCAN_TYPE - Find all subjects of type PERSON_CLASS
    int count0 = s7t_scan_by_type(engine, PERSON_CLASS, 
                                    reg0_ids, S7T_SQL_MAX_ROWS);

    // OP: SCAN_PREDICATE - Find all triples with predicate FOAF_KNOWS
    int count1 = s7t_scan_by_predicate(engine, FOAF_KNOWS,
                                         reg1_ids, S7T_SQL_MAX_ROWS);

    // OP: SCAN_TYPE - Find all subjects of type PERSON_CLASS
    int count2 = s7t_scan_by_type(engine, PERSON_CLASS, 
                                    reg2_ids, S7T_SQL_MAX_ROWS);

    // OP: HASH_JOIN - Join registers 0 and 1
    int count3 = s7t_hash_join(engine, 
                                 reg0_ids, count0,
                                 reg1_ids, count1,
                                 reg3_ids);

    // OP: PROJECT - Extract final result columns
    int final_count = (count3 < max_results) ? count3 : max_results;
    s7t_project_results(engine, reg3_ids, final_count, results);

    return final_count;
}

/**
 * @brief COMPILED SPARQL QUERY: organizationMembers
 * Estimated cycles: 9.2
 * 7-tick compliant: ❌ NO
 */
static inline int organizationMembers(
    CNSSparqlEngine* engine,
    QueryResult* results,
    int max_results
) {
    // Registers for intermediate results
    uint32_t reg0_ids[S7T_SQL_MAX_ROWS];
    float reg0_values[S7T_SQL_MAX_ROWS];
    uint32_t reg1_ids[S7T_SQL_MAX_ROWS];
    float reg1_values[S7T_SQL_MAX_ROWS];
    uint32_t reg2_ids[S7T_SQL_MAX_ROWS];
    float reg2_values[S7T_SQL_MAX_ROWS];
    uint32_t reg3_ids[S7T_SQL_MAX_ROWS];
    float reg3_values[S7T_SQL_MAX_ROWS];

    // Execute micro-operation tape

    // OP: SCAN_PREDICATE - Find all triples with predicate ID__memberOf
    int count0 = s7t_scan_by_predicate(engine, ID__memberOf,
                                         reg0_ids, S7T_SQL_MAX_ROWS);

    // OP: SCAN_TYPE - Find all subjects of type ORGANIZATION_CLASS
    int count1 = s7t_scan_by_type(engine, ORGANIZATION_CLASS, 
                                    reg1_ids, S7T_SQL_MAX_ROWS);

    // OP: HASH_JOIN - Join registers 0 and 1
    int count2 = s7t_hash_join(engine, 
                                 reg0_ids, count0,
                                 reg1_ids, count1,
                                 reg2_ids);

    // OP: PROJECT - Extract final result columns
    int final_count = (count2 < max_results) ? count2 : max_results;
    s7t_project_results(engine, reg2_ids, final_count, results);

    return final_count;
}


// Query dispatcher
static inline int execute_compiled_sparql_query(const char* query_name, CNSSparqlEngine* engine, QueryResult* results, int max_results) {
    if (strcmp(query_name, "getHighValueCustomers") == 0) {
        return getHighValueCustomers(engine, results, max_results);
    }
    if (strcmp(query_name, "findPersonsByName") == 0) {
        return findPersonsByName(engine, results, max_results);
    }
    if (strcmp(query_name, "getDocumentsByCreator") == 0) {
        return getDocumentsByCreator(engine, results, max_results);
    }
    if (strcmp(query_name, "socialConnections") == 0) {
        return socialConnections(engine, results, max_results);
    }
    if (strcmp(query_name, "organizationMembers") == 0) {
        return organizationMembers(engine, results, max_results);
    }
    return -1; // Query not found
}

#endif // SPARQL_QUERIES_AOT_H
