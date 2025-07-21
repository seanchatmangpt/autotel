/**
 * CNS v8 Automatic Signature Discovery Header
 * Gap 4 Solution: Real-time pattern recognition and signature generation
 * 
 * This header defines the structures and functions for automatically
 * discovering DSPy signatures from turtle stream patterns.
 */

#ifndef CNS_V8_AUTOMATIC_SIGNATURE_DISCOVERY_H
#define CNS_V8_AUTOMATIC_SIGNATURE_DISCOVERY_H

#include "cns_v8_dspy_owl_native_bridge.h"
#include <stdint.h>
#include <stdbool.h>

// ========================================
// PATTERN RECOGNITION STRUCTURES
// ========================================

// Maximum limits for pattern analysis
#define MAX_SIGNATURE_CANDIDATES 64
#define MAX_RECENT_TRIPLES 256
#define MAX_PATTERN_TYPES 8

// Pattern types for 80/20 analysis (from turtle_automation_gaps.json)
typedef enum {
    PATTERN_TYPE_DECL = 0,     // rdf:type statements (30%)
    PATTERN_LABEL = 1,         // rdfs:label statements (20%)
    PATTERN_PROPERTY = 2,      // Basic properties (20%)
    PATTERN_HIERARCHY = 3,     // subClassOf/subPropertyOf (10%)
    PATTERN_OTHER = 4,         // Everything else (20%)
    PATTERN_DSPY_INPUT = 5,    // DSPy input field patterns
    PATTERN_DSPY_OUTPUT = 6,   // DSPy output field patterns
    PATTERN_SIGNATURE = 7      // Complete signature patterns
} turtle_pattern_t;

// Candidate signature field discovered from patterns
typedef struct {
    char name[64];                 // Field name (e.g., "question", "answer")
    bool is_input;                 // Input field vs. output field
    uint8_t owl_datatype;          // Inferred OWL datatype
    uint8_t confidence;            // Confidence in this field (0-255)
    char description[128];         // Extracted field description
} signature_field_candidate_t;

// Complete signature pattern candidate
typedef struct {
    char signature_name[128];      // Inferred signature name
    double confidence;             // Overall confidence score (0.0-1.0)
    uint8_t field_count;           // Total fields discovered
    uint8_t input_count;           // Number of input fields
    uint8_t output_count;          // Number of output fields
    
    signature_field_candidate_t fields[16];  // Discovered fields
    
    // Discovery metadata
    uint64_t discovery_time;       // When pattern was first detected
    uint32_t observation_count;    // How many times pattern was observed
    double pattern_strength;       // How clear the pattern is
    
} signature_pattern_candidate_t;

// Adaptive pattern learning state
typedef struct {
    double learning_rate;          // Rate of adaptation (0.0-1.0)
    double adaptation_threshold;   // When to adapt patterns
    uint64_t last_adaptation_cycle; // Last adaptation time
    uint32_t adaptations_performed; // Total adaptations
    uint32_t successful_adaptations; // Successful adaptations
} pattern_adaptation_state_t;

// Main signature discovery engine
typedef struct {
    // Pattern frequency tracking (80/20 analysis)
    uint64_t observed_frequencies[MAX_PATTERN_TYPES];
    double expected_frequencies[MAX_PATTERN_TYPES];
    uint64_t total_triples_analyzed;
    
    // Signature discovery state
    signature_pattern_candidate_t candidates[MAX_SIGNATURE_CANDIDATES];
    uint8_t candidate_count;
    uint32_t min_pattern_count;    // Minimum observations before discovery
    double confidence_threshold;   // Minimum confidence for signature creation
    
    // Recent triple history for pattern matching
    triple_t recent_triples[MAX_RECENT_TRIPLES];
    uint16_t recent_triple_count;
    
    // Discovery engine configuration
    bool discovery_enabled;
    cns_cycle_t pattern_analysis_cycles;  // Budget for pattern analysis
    uint32_t discovery_interval;          // How often to check for patterns
    
    // Adaptive learning
    pattern_adaptation_state_t adaptation;
    
    // Performance metrics
    struct {
        uint64_t patterns_analyzed;
        uint64_t signatures_created;
        uint64_t adaptations_performed;
        uint64_t successful_adaptations;
        double total_confidence;
        cns_cycle_t total_analysis_cycles;
        uint32_t auto_discovered_signatures;
    } metrics;
    
    // Integration with parent bridge
    cns_v8_dspy_owl_bridge_t* parent_bridge;
    
} cns_v8_signature_discovery_t;

// ========================================
// DISCOVERY METRICS
// ========================================

typedef struct {
    uint64_t signatures_created;
    uint64_t patterns_analyzed;
    uint64_t adaptations_performed;
    uint64_t successful_adaptations;
    
    double discovery_success_rate;
    double avg_confidence;
    double adaptation_effectiveness;
    
    cns_cycle_t avg_analysis_cycles;
    cns_cycle_t total_discovery_time;
    
} signature_discovery_metrics_t;

// ========================================
// CORE API FUNCTIONS
// ========================================

// Initialize automatic signature discovery
int cns_v8_signature_discovery_init(
    cns_v8_signature_discovery_t* discovery,
    cns_v8_dspy_owl_bridge_t* parent_bridge,
    double confidence_threshold
);

// Analyze incoming triple for signature discovery opportunities
int cns_v8_analyze_triple_for_patterns(
    cns_v8_signature_discovery_t* discovery,
    const triple_t* triple,
    cns_cycle_t max_cycles
);

// Process batch of triples for pattern recognition
int cns_v8_analyze_triple_batch_for_patterns(
    cns_v8_signature_discovery_t* discovery,
    const triple_t* triples,
    size_t triple_count,
    cns_cycle_t max_total_cycles
);

// Force signature creation from current candidates
int cns_v8_force_signature_creation(
    cns_v8_signature_discovery_t* discovery,
    double minimum_confidence
);

// Get current signature discovery status
void cns_v8_get_discovery_status(
    const cns_v8_signature_discovery_t* discovery,
    uint8_t* active_candidates,
    double* avg_confidence,
    uint64_t* patterns_analyzed
);

// ========================================
// PATTERN ANALYSIS FUNCTIONS (INTERNAL)
// ========================================

// Classify triple into 80/20 pattern categories
static turtle_pattern_t classify_triple_pattern(const triple_t* triple);

// Detect potential DSPy signature patterns from triple sequences
static bool detect_signature_pattern(
    cns_v8_signature_discovery_t* discovery,
    const triple_t* triple,
    signature_pattern_candidate_t* candidate,
    cns_cycle_t max_cycles
);

// Calculate confidence score for signature pattern
static double calculate_pattern_confidence(
    cns_v8_signature_discovery_t* discovery,
    const triple_t* input_triple,
    const triple_t* output_triple
);

// Analyze field name patterns for DSPy compatibility
static double analyze_field_name_patterns(const char* predicate1, const char* predicate2);

// Analyze datatype compatibility between fields
static double analyze_datatype_compatibility(const char* object1, const char* object2);

// Analyze subject naming patterns for signature names
static double analyze_subject_naming_pattern(const char* subject);

// Calculate temporal clustering score for related triples
static double calculate_temporal_clustering_score(cns_v8_signature_discovery_t* discovery);

// Check for matches against known DSPy patterns
static double match_known_dspy_patterns(const triple_t* input_triple, const triple_t* output_triple);

// ========================================
// PATTERN CLASSIFICATION UTILITIES
// ========================================

// Detect if triple represents a property assertion
static bool is_property_assertion(const triple_t* triple);

// Check if field looks like DSPy input field
static bool is_potential_input_field(const triple_t* triple);

// Check if field looks like DSPy output field  
static bool is_potential_output_field(const triple_t* triple);

// Extract signature name from subject URI
static const char* extract_signature_name(const char* subject);

// Extract field name from predicate URI
static const char* extract_field_name(const char* predicate);

// Infer OWL datatype from object value
static uint8_t infer_owl_datatype(const char* object_value);

// Check semantic relationship between field names
static bool is_semantically_related(const char* name1, const char* name2);

// ========================================
// ADAPTIVE PATTERN LEARNING
// ========================================

// Update pattern frequencies and adapt thresholds
static void update_pattern_adaptation(cns_v8_signature_discovery_t* discovery);

// Adjust discovery parameters based on effectiveness
void cns_v8_adapt_discovery_parameters(
    cns_v8_signature_discovery_t* discovery,
    const signature_discovery_metrics_t* feedback
);

// Learn from successful signature creations
void cns_v8_learn_from_signature_success(
    cns_v8_signature_discovery_t* discovery,
    const native_dspy_owl_entity_t* successful_entity
);

// ========================================
// CANDIDATE MANAGEMENT
// ========================================

// Add signature candidate to discovery queue
static void add_signature_candidate(
    cns_v8_signature_discovery_t* discovery,
    const signature_pattern_candidate_t* candidate
);

// Remove least confident candidate to make space
static void remove_least_confident_candidate(cns_v8_signature_discovery_t* discovery);

// Check candidates for signature creation readiness
static void check_candidates_for_signature_creation(
    cns_v8_signature_discovery_t* discovery,
    cns_cycle_t max_cycles
);

// Create native DSPy signature from pattern candidate
static native_dspy_owl_entity_t* create_signature_from_candidate(
    cns_v8_signature_discovery_t* discovery,
    const signature_pattern_candidate_t* candidate,
    cns_cycle_t max_cycles
);

// ========================================
// SIGNATURE LIFECYCLE MANAGEMENT
// ========================================

// Register discovered signature with bridge
static void register_discovered_signature(
    cns_v8_dspy_owl_bridge_t* bridge,
    native_dspy_owl_entity_t* entity
);

// Log signature discovery for debugging/telemetry
static void log_signature_discovery(
    cns_v8_signature_discovery_t* discovery,
    const signature_pattern_candidate_t* candidate,
    const native_dspy_owl_entity_t* entity
);

// Remove candidate from discovery queue
static void remove_candidate(cns_v8_signature_discovery_t* discovery, int index);

// ========================================
// INTEGRATION WITH TURTLE LOOP
// ========================================

// Hook discovery into existing turtle processing loop
int cns_v8_register_discovery_with_turtle_loop(
    cns_v8_signature_discovery_t* discovery,
    cns_v8_automatic_turtle_loop_t* turtle_loop
);

// Process discovery during turtle loop execution
void cns_v8_discover_during_turtle_processing(
    cns_v8_signature_discovery_t* discovery,
    const triple_t* current_triple,
    cns_cycle_t remaining_budget
);

// Export discovered signatures for turtle loop use
int cns_v8_export_discovered_signatures(
    const cns_v8_signature_discovery_t* discovery,
    native_dspy_owl_entity_t** entities,
    size_t* entity_count
);

// ========================================
// PERFORMANCE MONITORING
// ========================================

// Get discovery performance metrics
void cns_v8_get_discovery_metrics(
    const cns_v8_signature_discovery_t* discovery,
    signature_discovery_metrics_t* metrics
);

// Monitor discovery performance in real-time
void cns_v8_monitor_discovery_performance(
    const cns_v8_signature_discovery_t* discovery,
    cns_cycle_t monitoring_interval
);

// ========================================
// UTILITY FUNCTIONS
// ========================================

// String hashing for fast lookups
uint32_t cns_v8_hash_string(const char* str);

// Get current CNS cycle time
cns_cycle_t cns_v8_get_current_cycle(void);

// Generate unique signature ID
static uint32_t generate_unique_signature_id(void);

// Generate unique SHACL shape ID
static uint32_t generate_unique_shape_id(void);

// Add triple to recent history buffer
static void add_triple_to_recent_history(
    cns_v8_signature_discovery_t* discovery,
    const triple_t* triple
);

// ========================================
// CONFIGURATION AND CLEANUP
// ========================================

// Load discovery configuration from file
int cns_v8_load_discovery_config(
    cns_v8_signature_discovery_t* discovery,
    const char* config_file_path
);

// Enable/disable automatic discovery
void cns_v8_set_discovery_enabled(
    cns_v8_signature_discovery_t* discovery,
    bool enabled
);

// Set discovery confidence threshold
void cns_v8_set_confidence_threshold(
    cns_v8_signature_discovery_t* discovery,
    double threshold
);

// Cleanup signature discovery system
void cns_v8_signature_discovery_cleanup(cns_v8_signature_discovery_t* discovery);

#endif // CNS_V8_AUTOMATIC_SIGNATURE_DISCOVERY_H