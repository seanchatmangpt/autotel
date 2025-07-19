#ifndef MCTS7T_H
#define MCTS7T_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <math.h>
#include <float.h>

// 7T Engine Integration
#include "c_src/sparql7t.h"
#include "c_src/shacl7t.h"
#include "c_src/owl7t.h"
#include "runtime/src/seven_t_runtime.h"

// MCTS Configuration
#define MCTS_MAX_DEPTH 50
#define MCTS_MAX_ITERATIONS 10000
#define MCTS_TIME_BUDGET_NS 100000000 // 100ms
#define MCTS_UCB_C 1.41421356237      // sqrt(2)
#define MCTS_SIMULATION_DEPTH 20
#define MCTS_EXPLORATION_CONSTANT 1.414

// MCTS Node Structure
typedef struct MCTSNode
{
  // State representation
  uint32_t *state_vector; // Current state as bit-vector
  size_t state_size;      // Size of state vector
  uint32_t depth;         // Current depth in tree

  // MCTS statistics
  double total_reward;   // Cumulative reward
  uint64_t visit_count;  // Number of visits
  double average_reward; // Cached average reward

  // Tree structure
  struct MCTSNode **children; // Child nodes
  size_t child_count;         // Number of children
  size_t child_capacity;      // Capacity of children array
  struct MCTSNode *parent;    // Parent node

  // Action information
  uint32_t action_id; // Action that led to this node
  double action_cost; // Cost of this action

  // 7T Engine specific
  EngineState *engine_state; // Reference to engine state
  uint32_t *pattern_order;   // Query pattern execution order
  size_t pattern_count;      // Number of patterns
} MCTSNode;

// MCTS Action Structure
typedef struct
{
  uint32_t action_id;      // Unique action identifier
  uint32_t action_type;    // Type of action (SPARQL, SHACL, OWL)
  uint32_t *parameters;    // Action parameters
  size_t param_count;      // Number of parameters
  double estimated_cost;   // Estimated cost of action
  double estimated_reward; // Estimated reward
} MCTSAction;

// MCTS Configuration
typedef struct
{
  uint32_t max_iterations;     // Maximum MCTS iterations
  uint64_t time_budget_ns;     // Time budget in nanoseconds
  uint32_t max_depth;          // Maximum tree depth
  uint32_t simulation_depth;   // Simulation rollout depth
  double exploration_constant; // UCB exploration constant
  uint64_t rng_seed;           // Random number generator seed
  bool enable_parallel;        // Enable parallel MCTS
  uint32_t num_threads;        // Number of parallel threads
} MCTSConfig;

// MCTS Result Structure
typedef struct
{
  uint32_t *best_actions;        // Best action sequence
  size_t action_count;           // Number of actions
  double total_reward;           // Total expected reward
  double confidence;             // Confidence in result
  uint64_t computation_time_ns;  // Time taken for computation
  uint64_t iterations_performed; // Number of iterations performed
} MCTSResult;

// MCTS Engine Structure
typedef struct
{
  MCTSConfig config;             // Configuration
  MCTSNode *root;                // Root node
  EngineState *engine;           // 7T engine state
  uint64_t *rng_state;           // Random number generator state
  uint64_t start_time_ns;        // Start time for time budget
  uint64_t iterations_completed; // Iterations completed
} MCTS7TEngine;

// Core MCTS Functions
MCTS7TEngine *mcts7t_create(EngineState *engine, MCTSConfig *config);
void mcts7t_destroy(MCTS7TEngine *mcts);

// Main MCTS Algorithm
MCTSResult *mcts7t_search(MCTS7TEngine *mcts, uint32_t *initial_state, size_t state_size);
bool mcts7t_step(MCTS7TEngine *mcts);

// MCTS Phases
MCTSNode *mcts7t_select(MCTS7TEngine *mcts, MCTSNode *node);
MCTSNode *mcts7t_expand(MCTS7TEngine *mcts, MCTSNode *node);
double mcts7t_simulate(MCTS7TEngine *mcts, MCTSNode *node);
void mcts7t_backpropagate(MCTS7TEngine *mcts, MCTSNode *node, double reward);

// Node Management
MCTSNode *mcts7t_create_node(uint32_t *state, size_t state_size, uint32_t depth, MCTSNode *parent);
void mcts7t_destroy_node(MCTSNode *node);
void mcts7t_add_child(MCTSNode *parent, MCTSNode *child);

// Action Generation
MCTSAction *mcts7t_generate_actions(MCTSNode *node, size_t *action_count);
void mcts7t_destroy_actions(MCTSAction *actions, size_t count);

// State Management
uint32_t *mcts7t_apply_action(uint32_t *state, size_t state_size, MCTSAction *action);
bool mcts7t_is_terminal(uint32_t *state, size_t state_size);

// Reward Functions
double mcts7t_calculate_reward(MCTSNode *node, MCTSAction *action);
double mcts7t_estimate_reward(uint32_t *state, size_t state_size, MCTSAction *action);

// 7T Engine Integration
double mcts7t_sparql_reward(EngineState *engine, uint32_t *patterns, size_t pattern_count);
double mcts7t_shacl_reward(EngineState *engine, uint32_t *shapes, size_t shape_count);
double mcts7t_owl_reward(EngineState *engine, uint32_t *axioms, size_t axiom_count);

// Utility Functions
double mcts7t_ucb_score(MCTSNode *node, double exploration_constant);
uint64_t mcts7t_get_time_ns(void);
uint32_t mcts7t_random_uint32(uint64_t *rng_state);
double mcts7t_random_double(uint64_t *rng_state);

// Performance Monitoring
typedef struct
{
  uint64_t selection_time_ns;
  uint64_t expansion_time_ns;
  uint64_t simulation_time_ns;
  uint64_t backpropagation_time_ns;
  uint64_t total_time_ns;
  uint64_t nodes_created;
  uint64_t nodes_visited;
  uint64_t simulations_performed;
} MCTSStats;

MCTSStats *mcts7t_get_stats(MCTS7TEngine *mcts);
void mcts7t_reset_stats(MCTS7TEngine *mcts);

// Default Configuration
extern const MCTSConfig MCTS7T_DEFAULT_CONFIG;

#endif // MCTS7T_H