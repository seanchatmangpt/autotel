/**
 * @file dark_feature_activator.h
 * @brief Dark Feature Activation System Header - Executable Ontology Engine
 * @version 2.0.0
 * 
 * This header defines the Dark Feature Activation System that transforms
 * dormant semantic knowledge into executable BitActor operations with
 * 95% ontology utilization and sub-100ns hot path performance.
 * 
 * CAUSAL ACTIVATION PRINCIPLE:
 * "Every piece of knowledge MUST be computationally active"
 * 
 * @author CausalActivator Agent - Sean Chatman Architecture
 * @date 2024-01-15
 */

#ifndef DARK_FEATURE_ACTIVATOR_H
#define DARK_FEATURE_ACTIVATOR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// DARK ACTIVATION CONSTANTS
// =============================================================================

#define DARK_ACTIVATOR_VERSION "2.0.0"
#define DARK_UTILIZATION_TARGET 95.0
#define BITACTOR_HOT_PATH_NS 62

// Forward declarations
typedef struct DarkFeatureActivator DarkFeatureActivator;

// Dark pattern types
typedef enum {
    DARK_PATTERN_UNUSED_TRIPLE = 0x01,
    DARK_PATTERN_HIDDEN_INFERENCE = 0x02,
    DARK_PATTERN_CAUSAL_LOOP = 0x03,
    DARK_PATTERN_SEMANTIC_BRIDGE = 0x04,
    DARK_PATTERN_ENTROPY_WELL = 0x05,
    DARK_PATTERN_QUANTUM_LINK = 0x06,
    DARK_PATTERN_RECURSIVE_DEPTH = 0x07,
    DARK_PATTERN_EMERGENT_LOGIC = 0x08
} DarkPatternType;

// Activation states
typedef enum {
    ACTIVATION_DORMANT = 0,
    ACTIVATION_COMPILING = 1,
    ACTIVATION_ACTIVE = 2,
    ACTIVATION_OPTIMIZED = 3,
    ACTIVATION_ENTANGLED = 4
} ActivationState;

// =============================================================================
// SYSTEM LIFECYCLE
// =============================================================================

/**
 * @brief Create dark feature activation system
 * @return Initialized activation system
 */
DarkFeatureActivator* dark_feature_activator_create(void);

/**
 * @brief Destroy activation system
 * @param activator System to destroy
 */
void dark_feature_activator_destroy(DarkFeatureActivator* activator);

// =============================================================================
// DARK PATTERN OPERATIONS
// =============================================================================

/**
 * @brief Discover and activate dark patterns from TTL specification
 * @param activator Dark feature activation system
 * @param ttl_specification TTL ontology specification
 * @return Number of patterns activated
 */
uint32_t dark_pattern_discover_and_activate(DarkFeatureActivator* activator, 
                                           const char* ttl_specification);

/**
 * @brief Execute dark feature activation with performance monitoring
 * @param activator Dark feature activation system
 * @param input_specification Input specification
 * @param[out] execution_time_ns Execution time in nanoseconds
 * @return Execution result hash
 */
uint64_t dark_feature_execute(DarkFeatureActivator* activator, 
                             const char* input_specification,
                             uint64_t* execution_time_ns);

// =============================================================================
// UTILIZATION MONITORING
// =============================================================================

/**
 * @brief Generate comprehensive utilization report
 * @param activator Dark feature activation system
 */
void dark_feature_generate_report(DarkFeatureActivator* activator);

/**
 * @brief Get current utilization percentage
 * @param activator Dark feature activation system
 * @return Current utilization percentage
 */
double dark_feature_get_utilization(DarkFeatureActivator* activator);

/**
 * @brief Check if utilization target is achieved
 * @param activator Dark feature activation system
 * @return true if 95% utilization target achieved
 */
bool dark_feature_target_achieved(DarkFeatureActivator* activator);

// =============================================================================
// DEMONSTRATION
// =============================================================================

/**
 * @brief Full demonstration of dark feature activation system
 */
void dark_feature_demonstration(void);

#ifdef __cplusplus
}
#endif

#endif // DARK_FEATURE_ACTIVATOR_H