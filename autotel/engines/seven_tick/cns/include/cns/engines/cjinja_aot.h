#ifndef CNS_CJINJA_AOT_H
#define CNS_CJINJA_AOT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @file cjinja_aot.h
 * @brief CJinja Ahead-of-Time Compilation Interface
 * 
 * This header provides the interface for AOT-compiled Jinja templates.
 * Templates are compiled at build time into native C functions for
 * maximum performance and 7-tick compliance.
 */

// Branch prediction hints for optimal performance
#define CJINJA_LIKELY(x) __builtin_expect(!!(x), 1)
#define CJINJA_UNLIKELY(x) __builtin_expect(!!(x), 0)

// Performance measurement
#define CJINJA_MEASURE_CYCLES(var) uint64_t var = __builtin_readcyclecounter()

// Template rendering result codes
typedef enum {
    CJINJA_OK = 0,
    CJINJA_ERROR_NULL_PARAM = -1,
    CJINJA_ERROR_BUFFER_TOO_SMALL = -2,
    CJINJA_ERROR_TEMPLATE_NOT_FOUND = -3,
    CJINJA_ERROR_CONTEXT_PARSE = -4,
    CJINJA_ERROR_INVALID_JSON = -5
} cjinja_result_t;

// Template metadata for runtime introspection
typedef struct {
    const char* name;
    const char* source_file;
    size_t context_size;
    int complexity_score;
    bool has_loops;
    bool has_conditionals;
    uint64_t compile_timestamp;
} cjinja_template_info_t;

// Performance metrics for rendered templates
typedef struct {
    uint64_t render_cycles;
    size_t bytes_written;
    bool exceeded_7tick_limit;
    const char* template_name;
} cjinja_perf_metrics_t;

/**
 * Core AOT Template Interface
 * These functions are implemented by the generated headers.
 */

/**
 * @brief Render a compiled template by name
 * @param template_name Name of the template to render
 * @param context Pointer to template-specific context struct
 * @param buffer Output buffer for rendered content
 * @param buffer_size Size of the output buffer
 * @return Number of bytes written, or negative error code
 */
extern int cjinja_render_template(
    const char* template_name,
    const void* context,
    char* buffer,
    size_t buffer_size
);

/**
 * @brief Get the size of context struct for a template
 * @param template_name Name of the template
 * @return Size in bytes, or 0 if template not found
 */
extern size_t cjinja_get_context_size(const char* template_name);

/**
 * @brief Get list of all available compiled templates
 * @return Null-terminated array of template names
 */
extern const char** cjinja_list_templates(void);

/**
 * @brief Parse JSON string into template context
 * @param template_name Name of the template
 * @param json_str JSON string containing context data
 * @param context Output context structure (pre-allocated)
 * @return true on success, false on error
 */
extern bool cjinja_parse_json_context(
    const char* template_name,
    const char* json_str,
    void* context
);

/**
 * Utility Functions
 */

/**
 * @brief Get metadata for a compiled template
 * @param template_name Name of the template
 * @return Template metadata, or NULL if not found
 */
extern const cjinja_template_info_t* cjinja_get_template_info(const char* template_name);

/**
 * @brief Render template with performance measurement
 * @param template_name Name of the template to render
 * @param context Template context
 * @param buffer Output buffer
 * @param buffer_size Size of output buffer
 * @param metrics Output performance metrics
 * @return Number of bytes written, or negative error code
 */
static inline int cjinja_render_with_metrics(
    const char* template_name,
    const void* context,
    char* buffer,
    size_t buffer_size,
    cjinja_perf_metrics_t* metrics
) {
    if (CJINJA_UNLIKELY(!metrics)) {
        return CJINJA_ERROR_NULL_PARAM;
    }
    
    // Measure rendering performance
    CJINJA_MEASURE_CYCLES(start_cycles);
    int result = cjinja_render_template(template_name, context, buffer, buffer_size);
    CJINJA_MEASURE_CYCLES(end_cycles);
    
    // Fill metrics
    metrics->render_cycles = end_cycles - start_cycles;
    metrics->bytes_written = (result > 0) ? (size_t)result : 0;
    metrics->exceeded_7tick_limit = (metrics->render_cycles > 7);
    metrics->template_name = template_name;
    
    return result;
}

/**
 * @brief Validate that template rendering is 7-tick compliant
 * @param template_name Name of template to test
 * @param context Template context
 * @param iterations Number of test iterations
 * @return true if all iterations complete within 7 cycles
 */
static inline bool cjinja_validate_7tick_compliance(
    const char* template_name,
    const void* context,
    int iterations
) {
    char test_buffer[4096];
    
    for (int i = 0; i < iterations; i++) {
        cjinja_perf_metrics_t metrics;
        int result = cjinja_render_with_metrics(
            template_name, context, test_buffer, sizeof(test_buffer), &metrics
        );
        
        if (result < 0 || metrics.exceeded_7tick_limit) {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief Create context from JSON file
 * @param template_name Name of the template
 * @param json_file Path to JSON file containing context data
 * @param context Output context structure (pre-allocated)
 * @return true on success, false on error
 */
extern bool cjinja_load_context_from_file(
    const char* template_name,
    const char* json_file,
    void* context
);

/**
 * @brief Allocate context structure for template
 * @param template_name Name of the template
 * @return Allocated context structure, or NULL on error
 * @note Caller must free the returned pointer
 */
extern void* cjinja_alloc_context(const char* template_name);

/**
 * @brief Free allocated context structure
 * @param context Context structure to free
 */
extern void cjinja_free_context(void* context);

/**
 * Debug and Development Utilities
 */

/**
 * @brief Print template compilation statistics
 * @param template_name Name of template, or NULL for all templates
 */
extern void cjinja_print_template_stats(const char* template_name);

/**
 * @brief Benchmark all compiled templates
 * @param iterations Number of iterations per template
 * @param print_results Whether to print results to stdout
 * @return Number of templates that failed 7-tick compliance
 */
extern int cjinja_benchmark_all_templates(int iterations, bool print_results);

/**
 * @brief Get total number of compiled templates
 * @return Number of templates available
 */
extern size_t cjinja_get_template_count(void);

/**
 * @brief Check if a template exists
 * @param template_name Name of the template
 * @return true if template is compiled and available
 */
static inline bool cjinja_template_exists(const char* template_name) {
    return cjinja_get_context_size(template_name) > 0;
}

/**
 * @brief Get human-readable error message
 * @param error_code Error code from cjinja_result_t
 * @return Human-readable error string
 */
static inline const char* cjinja_error_string(cjinja_result_t error_code) {
    switch (error_code) {
        case CJINJA_OK: return "Success";
        case CJINJA_ERROR_NULL_PARAM: return "Null parameter";
        case CJINJA_ERROR_BUFFER_TOO_SMALL: return "Buffer too small";
        case CJINJA_ERROR_TEMPLATE_NOT_FOUND: return "Template not found";
        case CJINJA_ERROR_CONTEXT_PARSE: return "Context parsing error";
        case CJINJA_ERROR_INVALID_JSON: return "Invalid JSON";
        default: return "Unknown error";
    }
}

/**
 * Configuration and Compilation Information
 */

// Compile-time configuration
#define CJINJA_AOT_VERSION_MAJOR 1
#define CJINJA_AOT_VERSION_MINOR 0
#define CJINJA_AOT_VERSION_PATCH 0

// Feature detection
#ifdef __has_builtin
    #if __has_builtin(__builtin_readcyclecounter)
        #define CJINJA_HAS_CYCLE_COUNTER 1
    #endif
#endif

// Compiler optimization hints
#if defined(__GNUC__) || defined(__clang__)
    #define CJINJA_FLATTEN __attribute__((flatten))
    #define CJINJA_HOT __attribute__((hot))
    #define CJINJA_PURE __attribute__((pure))
#else
    #define CJINJA_FLATTEN
    #define CJINJA_HOT
    #define CJINJA_PURE
#endif

/**
 * @brief Get CJinja AOT compiler version
 * @return Version string
 */
static inline const char* cjinja_aot_version(void) {
    return "1.0.0";
}

/**
 * @brief Get compilation timestamp
 * @return Unix timestamp of when templates were compiled
 */
extern uint64_t cjinja_compilation_timestamp(void);

#endif /* CNS_CJINJA_AOT_H */