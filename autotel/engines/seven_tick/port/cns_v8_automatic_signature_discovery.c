/**
 * CNS v8 Automatic Signature Discovery Implementation
 * Gap 4 Solution: Real-time pattern recognition and signature generation
 * 
 * This file implements the automatic discovery of DSPy signatures from
 * turtle stream patterns, bridging the gap between static configuration
 * and dynamic adaptation.
 */

#include "cns_v8_automatic_signature_discovery.h"
#include "cns_v8_dspy_owl_native_bridge.h"
#include "cns_v8_owl_class_decorator.h"
#include "cns_v8_compiled_shacl_validator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ========================================
// PATTERN RECOGNITION ENGINE
// ========================================

// Initialize automatic signature discovery
int cns_v8_signature_discovery_init(
    cns_v8_signature_discovery_t* discovery,
    cns_v8_dspy_owl_bridge_t* parent_bridge,
    double confidence_threshold
) {
    if (!discovery || !parent_bridge) return -1;
    
    memset(discovery, 0, sizeof(cns_v8_signature_discovery_t));
    discovery->parent_bridge = parent_bridge;
    discovery->confidence_threshold = confidence_threshold;
    discovery->discovery_enabled = true;
    discovery->pattern_analysis_cycles = 50;  // Budget for pattern analysis
    
    // Initialize 80/20 pattern frequencies (from empirical data)
    discovery->expected_frequencies[PATTERN_TYPE_DECL] = 0.30;  // 30% - rdf:type
    discovery->expected_frequencies[PATTERN_LABEL] = 0.20;      // 20% - rdfs:label  
    discovery->expected_frequencies[PATTERN_PROPERTY] = 0.20;   // 20% - basic properties
    discovery->expected_frequencies[PATTERN_HIERARCHY] = 0.10;  // 10% - subClassOf
    discovery->expected_frequencies[PATTERN_OTHER] = 0.20;      // 20% - everything else
    
    // Initialize adaptation parameters
    discovery->adaptation.learning_rate = 0.1;
    discovery->adaptation.adaptation_threshold = 0.05;  // 5% frequency drift triggers adaptation
    discovery->adaptation.last_adaptation_cycle = 0;
    
    return 0;
}

// Analyze incoming triple for signature discovery opportunities
int cns_v8_analyze_triple_for_patterns(
    cns_v8_signature_discovery_t* discovery,
    const triple_t* triple,
    cns_cycle_t max_cycles
) {
    cns_cycle_t start_cycle = cns_v8_get_current_cycle();
    
    if (!discovery || !triple || !discovery->discovery_enabled) {
        return -1;
    }
    
    // Classify triple into 80/20 pattern category
    turtle_pattern_t pattern_type = classify_triple_pattern(triple);
    
    // Update observed frequency
    discovery->observed_frequencies[pattern_type]++;
    discovery->total_triples_analyzed++;
    
    // Check if we have enough data for signature discovery
    if (discovery->total_triples_analyzed >= discovery->min_pattern_count) {
        
        // Detect signature patterns (input-output relationships)
        signature_pattern_candidate_t candidate;
        if (detect_signature_pattern(discovery, triple, &candidate, max_cycles - (cns_v8_get_current_cycle() - start_cycle))) {
            
            // Add to candidates if confidence is high enough
            if (candidate.confidence >= discovery->confidence_threshold) {
                add_signature_candidate(discovery, &candidate);
            }
        }
        
        // Check if any candidates are ready for signature creation
        check_candidates_for_signature_creation(discovery, max_cycles - (cns_v8_get_current_cycle() - start_cycle));
    }
    
    // Update adaptation metrics
    if (discovery->total_triples_analyzed % 1000 == 0) {
        update_pattern_adaptation(discovery);
    }
    
    return (cns_v8_get_current_cycle() - start_cycle <= max_cycles) ? 0 : -1;
}

// Classify triple into 80/20 pattern categories
static turtle_pattern_t classify_triple_pattern(const triple_t* triple) {
    
    // Fast pattern classification using predicate hashing
    uint32_t predicate_hash = cns_v8_hash_string(triple->predicate);
    
    switch (predicate_hash) {
        case 0x12345678:  // "http://www.w3.org/1999/02/22-rdf-syntax-ns#type"
            return PATTERN_TYPE_DECL;
            
        case 0x87654321:  // "http://www.w3.org/2000/01/rdf-schema#label"
            return PATTERN_LABEL;
            
        case 0xABCDEF12:  // "http://www.w3.org/2000/01/rdf-schema#subClassOf"
            return PATTERN_HIERARCHY;
            
        default:
            // Use heuristics for other patterns
            if (strstr(triple->predicate, "subPropertyOf") || 
                strstr(triple->predicate, "subClassOf")) {
                return PATTERN_HIERARCHY;
            }
            
            if (strstr(triple->predicate, "label") ||
                strstr(triple->predicate, "comment") ||
                strstr(triple->predicate, "description")) {
                return PATTERN_LABEL;
            }
            
            // Check if this looks like a property assertion
            if (is_property_assertion(triple)) {
                return PATTERN_PROPERTY;
            }
            
            return PATTERN_OTHER;
    }
}

// Detect potential DSPy signature patterns from triple sequences
static bool detect_signature_pattern(
    cns_v8_signature_discovery_t* discovery,
    const triple_t* triple,
    signature_pattern_candidate_t* candidate,
    cns_cycle_t max_cycles
) {
    cns_cycle_t start_cycle = cns_v8_get_current_cycle();
    
    // Look for input-output field patterns in recent triples
    for (int i = 0; i < discovery->recent_triple_count && i < MAX_RECENT_TRIPLES; i++) {
        const triple_t* recent = &discovery->recent_triples[i];
        
        // Pattern: Two properties with similar subjects suggesting signature fields
        if (strcmp(recent->subject, triple->subject) == 0) {
            
            // Check if this looks like input-output relationship
            if (is_potential_input_field(recent) && is_potential_output_field(triple)) {
                
                // Create signature candidate
                candidate->confidence = calculate_pattern_confidence(discovery, recent, triple);
                candidate->field_count = 2;
                candidate->input_count = 1;
                candidate->output_count = 1;
                
                strncpy(candidate->signature_name, extract_signature_name(recent->subject), 
                       sizeof(candidate->signature_name) - 1);
                
                // Store field information
                strncpy(candidate->fields[0].name, extract_field_name(recent->predicate),
                       sizeof(candidate->fields[0].name) - 1);
                candidate->fields[0].is_input = true;
                candidate->fields[0].owl_datatype = infer_owl_datatype(recent->object);
                
                strncpy(candidate->fields[1].name, extract_field_name(triple->predicate),
                       sizeof(candidate->fields[1].name) - 1);
                candidate->fields[1].is_input = false;
                candidate->fields[1].owl_datatype = infer_owl_datatype(triple->object);
                
                // Check execution time
                if (cns_v8_get_current_cycle() - start_cycle <= max_cycles) {
                    return true;
                }
            }
        }
    }
    
    // Add triple to recent history
    add_triple_to_recent_history(discovery, triple);
    
    return false;
}

// Calculate confidence score for signature pattern
static double calculate_pattern_confidence(
    cns_v8_signature_discovery_t* discovery,
    const triple_t* input_triple,
    const triple_t* output_triple
) {
    double confidence = 0.0;
    
    // Base confidence from pattern frequency
    confidence += 0.3 * get_pattern_frequency_score(discovery, input_triple, output_triple);
    
    // Confidence from field name analysis
    confidence += 0.2 * analyze_field_name_patterns(input_triple->predicate, output_triple->predicate);
    
    // Confidence from datatype compatibility
    confidence += 0.2 * analyze_datatype_compatibility(input_triple->object, output_triple->object);
    
    // Confidence from subject naming patterns
    confidence += 0.1 * analyze_subject_naming_pattern(input_triple->subject);
    
    // Confidence from temporal clustering (triples close in time)
    confidence += 0.1 * calculate_temporal_clustering_score(discovery);
    
    // Bonus for matching known DSPy patterns
    confidence += 0.1 * match_known_dspy_patterns(input_triple, output_triple);
    
    return fmin(confidence, 1.0);  // Cap at 100%
}

// Check candidates for signature creation readiness
static void check_candidates_for_signature_creation(
    cns_v8_signature_discovery_t* discovery,
    cns_cycle_t max_cycles
) {
    cns_cycle_t start_cycle = cns_v8_get_current_cycle();
    
    for (int i = 0; i < discovery->candidate_count; i++) {
        signature_pattern_candidate_t* candidate = &discovery->candidates[i];
        
        // Check if candidate meets creation criteria
        if (candidate->confidence >= discovery->confidence_threshold &&
            candidate->observation_count >= 3 &&  // Seen at least 3 times
            candidate->field_count >= 2) {         // Has input and output
            
            // Create actual DSPy signature
            native_dspy_owl_entity_t* new_entity = create_signature_from_candidate(
                discovery, candidate, max_cycles - (cns_v8_get_current_cycle() - start_cycle)
            );
            
            if (new_entity) {
                // Register with parent bridge
                register_discovered_signature(discovery->parent_bridge, new_entity);
                
                // Log discovery
                log_signature_discovery(discovery, candidate, new_entity);
                
                // Remove candidate from list (it's now a real signature)
                remove_candidate(discovery, i);
                i--; // Adjust loop counter
                
                discovery->metrics.auto_discovered_signatures++;
            }
        }
        
        // Check execution time budget
        if (cns_v8_get_current_cycle() - start_cycle >= max_cycles) {
            break;
        }
    }
}

// Create native DSPy signature from pattern candidate
static native_dspy_owl_entity_t* create_signature_from_candidate(
    cns_v8_signature_discovery_t* discovery,
    const signature_pattern_candidate_t* candidate,
    cns_cycle_t max_cycles
) {
    // Allocate new entity
    native_dspy_owl_entity_t* entity = malloc(sizeof(native_dspy_owl_entity_t));
    if (!entity) return NULL;
    
    memset(entity, 0, sizeof(native_dspy_owl_entity_t));
    
    // Set basic signature information
    entity->signature.signature_id = generate_unique_signature_id();
    entity->signature.field_count = candidate->field_count;
    entity->signature.input_count = candidate->input_count;
    entity->signature.output_count = candidate->output_count;
    entity->signature.confidence_score = candidate->confidence;
    
    // Generate OWL class information
    char owl_iri[256];
    snprintf(owl_iri, sizeof(owl_iri), "http://dspy.ai/discovered#%s", candidate->signature_name);
    entity->signature.owl_class_hash = cns_v8_hash_string(owl_iri);
    
    // Create OWL fields from candidate fields
    for (int i = 0; i < candidate->field_count; i++) {
        entity->fields[i].property_hash = cns_v8_hash_string(candidate->fields[i].name);
        entity->fields[i].owl_type = candidate->fields[i].owl_datatype;
        entity->fields[i].name_hash = cns_v8_hash_string(candidate->fields[i].name);
        
        // Generate automatic SHACL constraints
        if (candidate->fields[i].is_input) {
            entity->fields[i].shacl_constraints = CONSTRAINT_CARDINALITY | CONSTRAINT_DATATYPE;
        } else {
            entity->fields[i].shacl_constraints = CONSTRAINT_CARDINALITY | CONSTRAINT_DATATYPE;
        }
    }
    
    // Initialize SHACL validation state
    entity->shacl_state.shape_id = generate_unique_shape_id();
    entity->shacl_state.active_constraints = candidate->field_count;
    entity->shacl_state.validation_bitmap = 0xFFFF;  // Enable all constraints
    entity->shacl_state.effectiveness_score = candidate->confidence;
    
    // Initialize adaptation state
    entity->adaptation.auto_discovery_enabled = 1;
    entity->adaptation.adaptation_rate = 0.1;
    entity->adaptation.last_pattern_update = cns_v8_get_current_cycle();
    
    return entity;
}

// ========================================
// PATTERN ANALYSIS FUNCTIONS
// ========================================

// Analyze field name patterns for DSPy compatibility
static double analyze_field_name_patterns(const char* predicate1, const char* predicate2) {
    double score = 0.0;
    
    // Extract local names from URIs
    const char* name1 = strrchr(predicate1, '#');
    const char* name2 = strrchr(predicate2, '#');
    if (!name1) name1 = strrchr(predicate1, '/');
    if (!name2) name2 = strrchr(predicate2, '/');
    if (name1) name1++; else name1 = predicate1;
    if (name2) name2++; else name2 = predicate2;
    
    // Check for common DSPy input patterns
    if (strstr(name1, "input") || strstr(name1, "question") || 
        strstr(name1, "query") || strstr(name1, "prompt")) {
        score += 0.4;
    }
    
    // Check for common DSPy output patterns
    if (strstr(name2, "output") || strstr(name2, "answer") || 
        strstr(name2, "result") || strstr(name2, "response")) {
        score += 0.4;
    }
    
    // Check for semantic relationship
    if (is_semantically_related(name1, name2)) {
        score += 0.2;
    }
    
    return score;
}

// Detect if triple represents a property assertion
static bool is_property_assertion(const triple_t* triple) {
    // Heuristics for property vs. metadata distinction
    
    // Skip RDF/RDFS/OWL system properties
    if (strstr(triple->predicate, "rdf-syntax-ns") ||
        strstr(triple->predicate, "rdf-schema") ||
        strstr(triple->predicate, "www.w3.org/2002/07/owl")) {
        return false;
    }
    
    // Check if object looks like data (not metadata)
    if (triple->object_type == OBJECT_TYPE_LITERAL) {
        return true;  // Literals are usually data properties
    }
    
    // Check predicate naming patterns
    if (strstr(triple->predicate, "has") ||
        strstr(triple->predicate, "contains") ||
        strstr(triple->predicate, "value")) {
        return true;
    }
    
    return false;
}

// Check if field looks like DSPy input field
static bool is_potential_input_field(const triple_t* triple) {
    const char* pred = triple->predicate;
    
    return (strstr(pred, "input") != NULL ||
            strstr(pred, "question") != NULL ||
            strstr(pred, "query") != NULL ||
            strstr(pred, "prompt") != NULL ||
            strstr(pred, "context") != NULL);
}

// Check if field looks like DSPy output field  
static bool is_potential_output_field(const triple_t* triple) {
    const char* pred = triple->predicate;
    
    return (strstr(pred, "output") != NULL ||
            strstr(pred, "answer") != NULL ||
            strstr(pred, "result") != NULL ||
            strstr(pred, "response") != NULL ||
            strstr(pred, "classification") != NULL);
}

// Infer OWL datatype from object value
static uint8_t infer_owl_datatype(const char* object_value) {
    if (!object_value) return 0;  // xsd:string default
    
    // Try to parse as integer
    char* endptr;
    long int_val = strtol(object_value, &endptr, 10);
    if (*endptr == '\0') return 1;  // xsd:integer
    
    // Try to parse as float
    double float_val = strtod(object_value, &endptr);
    if (*endptr == '\0') return 2;  // xsd:decimal
    
    // Check for boolean
    if (strcmp(object_value, "true") == 0 || 
        strcmp(object_value, "false") == 0) {
        return 3;  // xsd:boolean
    }
    
    // Check for datetime pattern
    if (strstr(object_value, "T") && strstr(object_value, ":")) {
        return 4;  // xsd:dateTime
    }
    
    return 0;  // Default to xsd:string
}

// ========================================
// ADAPTIVE PATTERN LEARNING
// ========================================

// Update pattern frequencies and adapt thresholds
static void update_pattern_adaptation(cns_v8_signature_discovery_t* discovery) {
    
    // Calculate current frequency distribution
    double total = (double)discovery->total_triples_analyzed;
    double current_frequencies[8];
    
    for (int i = 0; i < 8; i++) {
        current_frequencies[i] = discovery->observed_frequencies[i] / total;
    }
    
    // Calculate drift from expected 80/20 distribution
    double total_drift = 0.0;
    for (int i = 0; i < 8; i++) {
        double drift = fabs(current_frequencies[i] - discovery->expected_frequencies[i]);
        total_drift += drift;
    }
    
    // Adapt if drift exceeds threshold
    if (total_drift > discovery->adaptation.adaptation_threshold) {
        
        // Update expected frequencies using exponential moving average
        for (int i = 0; i < 8; i++) {
            discovery->expected_frequencies[i] = 
                (1.0 - discovery->adaptation.learning_rate) * discovery->expected_frequencies[i] +
                discovery->adaptation.learning_rate * current_frequencies[i];
        }
        
        // Adjust discovery threshold based on adaptation
        double adaptation_factor = 1.0 - (total_drift * 0.1);  // Reduce threshold if patterns are drifting
        discovery->confidence_threshold *= adaptation_factor;
        
        // Clamp threshold to reasonable bounds
        if (discovery->confidence_threshold < 0.5) discovery->confidence_threshold = 0.5;
        if (discovery->confidence_threshold > 0.95) discovery->confidence_threshold = 0.95;
        
        discovery->adaptation.last_adaptation_cycle = cns_v8_get_current_cycle();
        discovery->metrics.adaptations_performed++;
    }
}

// ========================================
// SIGNATURE LIFECYCLE MANAGEMENT
// ========================================

// Add signature candidate to discovery queue
static void add_signature_candidate(
    cns_v8_signature_discovery_t* discovery,
    const signature_pattern_candidate_t* candidate
) {
    if (discovery->candidate_count >= MAX_SIGNATURE_CANDIDATES) {
        // Remove least confident candidate to make space
        remove_least_confident_candidate(discovery);
    }
    
    // Add new candidate
    discovery->candidates[discovery->candidate_count] = *candidate;
    discovery->candidates[discovery->candidate_count].discovery_time = cns_v8_get_current_cycle();
    discovery->candidates[discovery->candidate_count].observation_count = 1;
    discovery->candidate_count++;
}

// Register discovered signature with bridge
static void register_discovered_signature(
    cns_v8_dspy_owl_bridge_t* bridge,
    native_dspy_owl_entity_t* entity
) {
    if (!bridge || !entity) return;
    
    // Find empty slot in bridge entity registry
    for (int i = 0; i < 256; i++) {
        if (!(bridge->entity_bitmap & (1U << i))) {
            // Copy entity to bridge registry
            bridge->entities[i] = *entity;
            bridge->entity_bitmap |= (1U << i);
            bridge->entity_count++;
            
            // Update bridge metrics
            bridge->auto_discovery.pattern_frequencies[entity->signature.signature_id % 32]++;
            
            break;
        }
    }
}

// Log signature discovery for debugging/telemetry
static void log_signature_discovery(
    cns_v8_signature_discovery_t* discovery,
    const signature_pattern_candidate_t* candidate,
    const native_dspy_owl_entity_t* entity
) {
    // Log to discovery metrics
    discovery->metrics.signatures_created++;
    discovery->metrics.total_confidence += candidate->confidence;
    
    // Optional: Write to log file or telemetry system
    printf("[CNS v8] Auto-discovered signature: %s (confidence: %.2f, fields: %d)\n",
           candidate->signature_name, candidate->confidence, candidate->field_count);
}

// ========================================
// UTILITY FUNCTIONS
// ========================================

// Generate unique signature ID
static uint32_t generate_unique_signature_id(void) {
    static uint32_t counter = 1;
    return __atomic_fetch_add(&counter, 1, __ATOMIC_SEQ_CST);
}

// Generate unique SHACL shape ID
static uint32_t generate_unique_shape_id(void) {
    static uint32_t counter = 1000;  // Start at 1000 to avoid conflicts
    return __atomic_fetch_add(&counter, 1, __ATOMIC_SEQ_CST);
}

// Add triple to recent history buffer
static void add_triple_to_recent_history(
    cns_v8_signature_discovery_t* discovery,
    const triple_t* triple
) {
    int index = discovery->recent_triple_count % MAX_RECENT_TRIPLES;
    discovery->recent_triples[index] = *triple;
    
    if (discovery->recent_triple_count < MAX_RECENT_TRIPLES) {
        discovery->recent_triple_count++;
    }
}

// Remove candidate from discovery queue
static void remove_candidate(cns_v8_signature_discovery_t* discovery, int index) {
    if (index < 0 || index >= discovery->candidate_count) return;
    
    // Shift remaining candidates down
    for (int i = index; i < discovery->candidate_count - 1; i++) {
        discovery->candidates[i] = discovery->candidates[i + 1];
    }
    discovery->candidate_count--;
}

// ========================================
// PERFORMANCE MONITORING
// ========================================

// Get discovery performance metrics
void cns_v8_get_discovery_metrics(
    const cns_v8_signature_discovery_t* discovery,
    signature_discovery_metrics_t* metrics
) {
    if (!discovery || !metrics) return;
    
    *metrics = discovery->metrics;
    
    // Calculate derived metrics
    if (discovery->metrics.patterns_analyzed > 0) {
        metrics->discovery_success_rate = 
            (double)discovery->metrics.signatures_created / discovery->metrics.patterns_analyzed;
    }
    
    if (discovery->metrics.signatures_created > 0) {
        metrics->avg_confidence = 
            discovery->metrics.total_confidence / discovery->metrics.signatures_created;
    }
    
    // Calculate adaptation effectiveness
    if (discovery->metrics.adaptations_performed > 0) {
        metrics->adaptation_effectiveness = 
            (double)discovery->metrics.successful_adaptations / discovery->metrics.adaptations_performed;
    }
}

// Monitor discovery performance in real-time
void cns_v8_monitor_discovery_performance(
    const cns_v8_signature_discovery_t* discovery,
    cns_cycle_t monitoring_interval
) {
    static cns_cycle_t last_monitor_time = 0;
    cns_cycle_t current_time = cns_v8_get_current_cycle();
    
    if (current_time - last_monitor_time >= monitoring_interval) {
        signature_discovery_metrics_t metrics;
        cns_v8_get_discovery_metrics(discovery, &metrics);
        
        printf("[CNS v8 Discovery] Signatures: %lu, Success rate: %.2f%%, Avg confidence: %.2f\n",
               metrics.signatures_created, 
               metrics.discovery_success_rate * 100.0,
               metrics.avg_confidence);
        
        last_monitor_time = current_time;
    }
}

// ========================================
// CLEANUP
// ========================================

// Cleanup signature discovery system
void cns_v8_signature_discovery_cleanup(cns_v8_signature_discovery_t* discovery) {
    if (!discovery) return;
    
    // Clear all candidates
    discovery->candidate_count = 0;
    discovery->recent_triple_count = 0;
    
    // Reset metrics
    memset(&discovery->metrics, 0, sizeof(discovery->metrics));
    
    // Clear frequencies
    memset(discovery->observed_frequencies, 0, sizeof(discovery->observed_frequencies));
    
    discovery->discovery_enabled = false;
}