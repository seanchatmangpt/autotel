/**
 * CNS v8 Turtle Loop ML Optimizer
 * Neural pattern prediction and dynamic 80/20 optimization
 * Ensures Pareto efficiency remains above 0.85
 */

#ifndef CNS_V8_TURTLE_LOOP_ML_OPTIMIZER_H
#define CNS_V8_TURTLE_LOOP_ML_OPTIMIZER_H

#include "cns_v8_turtle_loop_integration.h"
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

// ML-based pattern predictor
typedef struct {
    // Neural network weights for pattern prediction (7-tick compliant)
    double pattern_weights[5][8];      // 5 patterns x 8 features
    double pattern_bias[5];            // Bias for each pattern
    double learning_rate;              // Adaptive learning rate
    
    // Historical pattern data for training
    uint64_t pattern_history[5][100]; // Circular buffer for each pattern
    uint32_t history_index;
    uint32_t history_size;
    
    // Prediction accuracy tracking
    uint64_t predictions_made;
    uint64_t predictions_correct;
    double accuracy;
} pattern_predictor_t;

// Dynamic 80/20 optimizer
typedef struct {
    // Current Pareto split
    double current_split;              // 0.8 = 80/20, adjustable
    double target_efficiency;          // Target Pareto efficiency (0.85)
    
    // Dynamic thresholds
    uint64_t pattern_thresholds[5];    // Minimum occurrences to be "vital"
    uint64_t stage_cycle_limits[6];    // Max cycles per stage
    
    // Performance tracking
    double rolling_efficiency;         // Exponential moving average
    double efficiency_gradient;        // Rate of change
    uint32_t recalc_counter;          // Recalculation frequency
} pareto_optimizer_t;

// Feedback loop controller
typedef struct {
    // Performance metrics
    double throughput_rate;            // Triples per cycle
    double prediction_latency;         // Cycles for prediction
    double optimization_overhead;      // Cycles for optimization
    
    // Adaptive parameters
    double momentum;                   // Learning momentum
    double exploration_rate;           // Exploration vs exploitation
    uint32_t feedback_interval;        // Cycles between feedback
    
    // Self-improvement tracking
    uint64_t improvement_cycles;
    double cumulative_gain;
} feedback_controller_t;

// ML-enhanced Turtle Loop
typedef struct {
    // Base turtle loop
    cns_v8_turtle_loop_t* base_loop;
    
    // ML components
    pattern_predictor_t predictor;
    pareto_optimizer_t optimizer;
    feedback_controller_t feedback;
    
    // Real-time metrics
    struct {
        uint64_t total_predictions;
        uint64_t accurate_predictions;
        double current_pareto_efficiency;
        uint64_t optimization_cycles;
        uint64_t ml_overhead_cycles;
    } metrics;
    
    // Configuration
    struct {
        bool enable_prediction;
        bool enable_dynamic_optimization;
        bool enable_feedback_loop;
        uint32_t min_samples_for_prediction;
        double min_confidence_threshold;
    } config;
} cns_v8_ml_turtle_loop_t;

// Initialize ML-enhanced turtle loop
int cns_v8_ml_turtle_loop_init(
    cns_v8_ml_turtle_loop_t* ml_loop,
    cns_v8_turtle_loop_t* base_loop,
    double target_efficiency
);

// Train pattern predictor on historical data
int cns_v8_train_pattern_predictor(
    cns_v8_ml_turtle_loop_t* ml_loop,
    const triple_pattern_t* patterns,
    size_t pattern_count
);

// Predict next pattern (7-tick compliant)
triple_pattern_t cns_v8_predict_next_pattern(
    cns_v8_ml_turtle_loop_t* ml_loop,
    const triple_pattern_t* recent_patterns,
    size_t recent_count,
    double* confidence
);

// Dynamically recalculate 80/20 split
int cns_v8_recalculate_pareto_split(
    cns_v8_ml_turtle_loop_t* ml_loop,
    const cns_v8_metrics_t* current_metrics
);

// Apply feedback for self-improvement
int cns_v8_apply_feedback_loop(
    cns_v8_ml_turtle_loop_t* ml_loop,
    double performance_delta
);

// Process turtle with ML optimization
int cns_v8_ml_process_turtle(
    cns_v8_ml_turtle_loop_t* ml_loop,
    const char* turtle_data,
    size_t data_size,
    void* output_buffer,
    size_t* output_size
);

// Get enhanced metrics
typedef struct {
    cns_v8_metrics_t base_metrics;
    double prediction_accuracy;
    double optimization_efficiency;
    double feedback_improvement_rate;
    double ml_overhead_percent;
    uint64_t vital_few_identified;
    uint64_t trivial_many_skipped;
} cns_v8_ml_metrics_t;

void cns_v8_get_ml_metrics(
    const cns_v8_ml_turtle_loop_t* ml_loop,
    cns_v8_ml_metrics_t* metrics
);

// Optimization strategies
typedef enum {
    OPT_STRATEGY_CONSERVATIVE = 0,  // Maintain 80/20 strictly
    OPT_STRATEGY_AGGRESSIVE   = 1,  // Push to 90/10 if beneficial
    OPT_STRATEGY_ADAPTIVE     = 2,  // Let ML decide split
    OPT_STRATEGY_QUANTUM      = 3   // 8-aligned optimization
} optimization_strategy_t;

void cns_v8_set_optimization_strategy(
    cns_v8_ml_turtle_loop_t* ml_loop,
    optimization_strategy_t strategy
);

// Neural network operations (7-tick compliant)
static inline double sigmoid_7tick(double x) {
    // Fast sigmoid approximation
    return x / (1.0 + fabs(x));
}

static inline double tanh_7tick(double x) {
    // Fast tanh approximation
    double x2 = x * x;
    return x * (27.0 + x2) / (27.0 + 9.0 * x2);
}

// Cleanup
void cns_v8_ml_turtle_loop_cleanup(cns_v8_ml_turtle_loop_t* ml_loop);

#endif // CNS_V8_TURTLE_LOOP_ML_OPTIMIZER_H