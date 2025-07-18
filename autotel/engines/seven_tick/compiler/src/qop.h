#ifndef QOP_H
#define QOP_H

#include <stdint.h>
#include <stddef.h>

// Query pattern representation
typedef struct {
    int32_t subject;   // -1 for variable
    int32_t predicate; // -1 for variable
    int32_t object;    // -1 for variable
    int subject_var_idx;
    int predicate_var_idx;
    int object_var_idx;
} Pattern;

// Join order plan
typedef struct {
    int* order;         // Pattern execution order
    double cost;        // Estimated cost
    size_t length;      // Number of patterns
} JoinPlan;

// MCTS node
typedef struct MCTSNode {
    int* partial_order;
    size_t depth;
    int* remaining;
    size_t remaining_count;
    double total_reward;
    size_t visit_count;
    struct MCTSNode** children;
    size_t child_count;
    struct MCTSNode* parent;
} MCTSNode;

// Cost model statistics
typedef struct {
    size_t* predicate_cardinalities;
    size_t* object_cardinalities;
    double* predicate_selectivities;
    size_t total_triples;
    size_t max_predicate_id;
    size_t max_object_id;
} CostModel;

// Main MCTS optimizer
JoinPlan* mcts_optimize_query(Pattern* patterns, size_t pattern_count, 
                             CostModel* cost_model, size_t iterations);

// Cost model creation
CostModel* create_cost_model(void* engine_state);
void destroy_cost_model(CostModel* model);

// Helper functions
double estimate_pattern_cost(Pattern* pattern, CostModel* model);
double estimate_join_cost(Pattern* p1, Pattern* p2, double card1, double card2);

#endif // QOP_H
