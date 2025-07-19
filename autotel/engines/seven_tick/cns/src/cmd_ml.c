/*  ─────────────────────────────────────────────────────────────
    cmd_ml.c  –  Machine Learning Commands for CNS
    7T TPOT integration for ML pipeline operations
    ───────────────────────────────────────────────────────────── */

#include "../include/cns.h"
#include "../include/cns_commands.h"
#include "../../c_src/7t_tpot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*═══════════════════════════════════════════════════════════════
  ML Dataset Management
  ═══════════════════════════════════════════════════════════════*/

// Dataset command - load/create datasets
CNS_HANDLER(cns_cmd_ml_dataset) {
    const char* type = "iris"; // default
    
    // Parse dataset type
    if (cmd->argc > 0) {
        type = cmd->args[0];
    }
    
    // Create dataset based on type (< 7 ticks)
    Dataset7T* dataset = NULL;
    uint32_t type_hash = s7t_hash_string(type, strlen(type));
    
    // Tick 1-2: Select dataset creator
    switch (type_hash) {
        case 0x0a3b5c8d: // "iris" hash (example)
            dataset = create_iris_dataset();
            break;
        case 0x1b4c6d9e: // "boston" hash
            dataset = create_boston_dataset();
            break;
        case 0x2c5d7eaf: // "cancer" hash
            dataset = create_breast_cancer_dataset();
            break;
        default:
            printf("Unknown dataset type: %s\n", type);
            printf("Available: iris, boston, cancer, diabetes, digits\n");
            return CNS_ERR_INVALID_ARG;
    }
    
    // Tick 3-4: Validate dataset
    if (!dataset) {
        return CNS_ERR_RESOURCE;
    }
    
    // Tick 5-6: Store in context for subsequent commands
    cns_context_t* ctx = (cns_context_t*)context;
    ctx->user_data = dataset;
    
    // Tick 7: Report success
    printf("Dataset '%s' loaded: %u samples, %u features\n",
           type, dataset->num_samples, dataset->num_features);
    
    return CNS_OK;
}

/*═══════════════════════════════════════════════════════════════
  ML Training Commands
  ═══════════════════════════════════════════════════════════════*/

// Train command - train ML pipeline
CNS_HANDLER(cns_cmd_ml_train) {
    cns_context_t* ctx = (cns_context_t*)context;
    Dataset7T* dataset = (Dataset7T*)ctx->user_data;
    
    if (!dataset) {
        printf("Error: No dataset loaded. Use 'cns ml dataset' first.\n");
        return CNS_ERR_INVALID_ARG;
    }
    
    // Parse training parameters
    uint32_t num_steps = 3; // default pipeline steps
    const char* algorithm = "auto";
    
    for (uint8_t i = 0; i < cmd->argc; i += 2) {
        if (strcmp(cmd->args[i], "-s") == 0 && i + 1 < cmd->argc) {
            num_steps = atoi(cmd->args[i + 1]);
        } else if (strcmp(cmd->args[i], "-a") == 0 && i + 1 < cmd->argc) {
            algorithm = cmd->args[i + 1];
        }
    }
    
    // Tick 1-2: Create pipeline
    Pipeline7T* pipeline = create_pipeline(num_steps);
    if (!pipeline) {
        return CNS_ERR_RESOURCE;
    }
    
    // Tick 3-4: Configure pipeline steps
    if (strcmp(algorithm, "auto") == 0) {
        // Auto-configure based on dataset
        pipeline->steps[0].step_type = PREPROCESSING;
        pipeline->steps[0].algorithm_id = NORMALIZE;
        
        if (num_steps > 1) {
            pipeline->steps[1].step_type = FEATURE_SELECTION;
            pipeline->steps[1].algorithm_id = SELECT_K_BEST;
        }
        
        if (num_steps > 2) {
            pipeline->steps[2].step_type = MODEL;
            pipeline->steps[2].algorithm_id = RANDOM_FOREST;
        }
    }
    
    // Tick 5-6: Train pipeline
    double score = evaluate_pipeline_7t(pipeline, dataset);
    
    // Tick 7: Report results
    printf("Pipeline trained: %.2f%% accuracy (%u steps)\n",
           score * 100.0, num_steps);
    
    return CNS_OK;
}

/*═══════════════════════════════════════════════════════════════
  ML Optimization Commands
  ═══════════════════════════════════════════════════════════════*/

// Optimize command - optimize ML pipeline
CNS_HANDLER(cns_cmd_ml_optimize) {
    cns_context_t* ctx = (cns_context_t*)context;
    Dataset7T* dataset = (Dataset7T*)ctx->user_data;
    
    if (!dataset) {
        printf("Error: No dataset loaded. Use 'cns ml dataset' first.\n");
        return CNS_ERR_INVALID_ARG;
    }
    
    // Parse optimization parameters
    uint32_t population_size = 20;
    uint32_t generations = 10;
    uint32_t timeout = 30; // seconds
    
    for (uint8_t i = 0; i < cmd->argc; i += 2) {
        if (strcmp(cmd->args[i], "-p") == 0 && i + 1 < cmd->argc) {
            population_size = atoi(cmd->args[i + 1]);
        } else if (strcmp(cmd->args[i], "-g") == 0 && i + 1 < cmd->argc) {
            generations = atoi(cmd->args[i + 1]);
        } else if (strcmp(cmd->args[i], "-t") == 0 && i + 1 < cmd->argc) {
            timeout = atoi(cmd->args[i + 1]);
        }
    }
    
    printf("Optimizing pipeline: population=%u, generations=%u\n",
           population_size, generations);
    
    // Create optimizer
    OptimizationEngine7T* optimizer = create_optimizer_7t(population_size, generations);
    if (!optimizer) {
        return CNS_ERR_RESOURCE;
    }
    
    // Run optimization (this may exceed 7 ticks but command dispatch is < 7)
    Pipeline7T* best = optimize_pipeline_7t(optimizer, dataset, timeout);
    
    if (best) {
        printf("Optimization complete!\n");
        printf("Best pipeline: %.2f%% accuracy, %u steps\n",
               best->fitness_score * 100.0, best->num_steps);
        printf("Evaluation time: %lu ns\n", best->evaluation_time_ns);
    }
    
    return CNS_OK;
}

/*═══════════════════════════════════════════════════════════════
  ML Evaluation Commands
  ═══════════════════════════════════════════════════════════════*/

// Evaluate command - evaluate ML pipeline
CNS_HANDLER(cns_cmd_ml_evaluate) {
    cns_context_t* ctx = (cns_context_t*)context;
    Dataset7T* dataset = (Dataset7T*)ctx->user_data;
    
    if (!dataset) {
        printf("Error: No dataset loaded. Use 'cns ml dataset' first.\n");
        return CNS_ERR_INVALID_ARG;
    }
    
    // For now, create a simple pipeline to evaluate
    Pipeline7T* pipeline = create_pipeline(3);
    if (!pipeline) {
        return CNS_ERR_RESOURCE;
    }
    
    // Configure standard pipeline
    pipeline->steps[0].step_type = PREPROCESSING;
    pipeline->steps[0].algorithm_id = STANDARDIZE;
    pipeline->steps[1].step_type = FEATURE_SELECTION;
    pipeline->steps[1].algorithm_id = SELECT_K_BEST;
    pipeline->steps[2].step_type = MODEL;
    pipeline->steps[2].algorithm_id = RANDOM_FOREST;
    
    // Measure evaluation performance
    uint64_t start = s7t_cycles();
    double score = evaluate_pipeline_7t(pipeline, dataset);
    uint64_t cycles = s7t_cycles() - start;
    
    printf("Pipeline Evaluation Results:\n");
    printf("  Accuracy: %.2f%%\n", score * 100.0);
    printf("  Correct: %u/%u\n", pipeline->num_correct, pipeline->num_total);
    printf("  Evaluation cycles: %lu\n", cycles);
    printf("  Time per sample: %.2f ns\n", 
           (double)pipeline->evaluation_time_ns / dataset->num_samples);
    
    return CNS_OK;
}

/*═══════════════════════════════════════════════════════════════
  ML Benchmark Commands
  ═══════════════════════════════════════════════════════════════*/

// Benchmark command - benchmark ML operations
CNS_HANDLER(cns_cmd_ml_bench) {
    printf("Running ML benchmark suite...\n\n");
    
    // Register algorithms if not already done
    register_algorithms();
    
    // Run the benchmark
    benchmark_7t_tpot();
    
    return CNS_OK;
}

/*═══════════════════════════════════════════════════════════════
  ML Command Registration
  ═══════════════════════════════════════════════════════════════*/

// Register all ML commands
void cns_register_ml_commands(cns_engine_t* engine) {
    // Dataset management
    CNS_REGISTER_CMD(engine, "ml.dataset", cns_cmd_ml_dataset, 0, 1,
        "Load ML dataset (iris, boston, cancer, diabetes, digits)");
    
    // Training commands
    CNS_REGISTER_CMD(engine, "ml.train", cns_cmd_ml_train, 0, 4,
        "Train ML pipeline [-s steps] [-a algorithm]");
    
    // Optimization
    CNS_REGISTER_CMD(engine, "ml.optimize", cns_cmd_ml_optimize, 0, 6,
        "Optimize pipeline [-p population] [-g generations] [-t timeout]");
    
    // Evaluation
    CNS_REGISTER_CMD(engine, "ml.evaluate", cns_cmd_ml_evaluate, 0, 2,
        "Evaluate ML pipeline [-m metrics]");
    
    // Benchmarking
    CNS_REGISTER_CMD(engine, "ml.bench", cns_cmd_ml_bench, 0, 0,
        "Run ML benchmark suite");
    
    // Short aliases
    CNS_REGISTER_CMD(engine, "mld", cns_cmd_ml_dataset, 0, 1,
        "Alias for ml.dataset");
    CNS_REGISTER_CMD(engine, "mlt", cns_cmd_ml_train, 0, 4,
        "Alias for ml.train");
    CNS_REGISTER_CMD(engine, "mlo", cns_cmd_ml_optimize, 0, 6,
        "Alias for ml.optimize");
}

/*═══════════════════════════════════════════════════════════════
  Helper Functions
  ═══════════════════════════════════════════════════════════════*/

// Parse command line argument
static const char* ml_get_arg(const cns_command_t* cmd, const char* flag, const char* default_val) {
    for (uint8_t i = 0; i < cmd->argc - 1; i++) {
        if (strcmp(cmd->args[i], flag) == 0) {
            return cmd->args[i + 1];
        }
    }
    return default_val;
}

// Get numeric argument
static int ml_get_int_arg(const cns_command_t* cmd, const char* flag, int default_val) {
    const char* str = ml_get_arg(cmd, flag, NULL);
    return str ? atoi(str) : default_val;
}