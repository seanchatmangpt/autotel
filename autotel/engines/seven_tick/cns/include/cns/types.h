#ifndef CNS_TYPES_H
#define CNS_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include "cns/commands/base.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 7T SUBSTRATE CORE TYPES - 7-TICK PERFORMANCE CONSTRAINTS
// ============================================================================

// 7T Performance guarantee: All operations must complete within 7 CPU ticks
#define CNS_7T_TICK_LIMIT 7
#define CNS_7T_CACHE_LINE_SIZE 64
#define CNS_7T_ALIGNMENT 64

// ============================================================================
// 8T SUBSTRATE INTEGRATION - EXTENDED PERFORMANCE CAPABILITIES
// ============================================================================

// 8T extensions are available when compiled with CNS_8T_ENABLED
#ifdef CNS_8T_ENABLED
#include "cns/8t.h"

// 8T Performance guarantee: All operations must complete within 8 CPU ticks
#define CNS_8T_TICK_LIMIT 8
#define CNS_8T_SIMD_WIDTH 32

// Extended context with 8T capabilities
typedef struct {
    cns_context_t base_7t;      // Base 7T context
    cns_8t_system_t* system_8t; // 8T extended system
    bool compatibility_mode;    // 7T compatibility mode
    uint32_t feature_flags;     // 8T feature flags
} cns_extended_context_t;

#endif // CNS_8T_ENABLED

// Core 7T substrate identifier - 64-bit for deterministic memory layout
typedef uint64_t cns_id_t;
typedef uint32_t cns_type_id_t;
typedef uint32_t cns_hash_t;

// 7T tick counter for performance tracking
typedef uint64_t cns_tick_t;

// 7T arena allocation handle - O(1) allocation and deallocation
typedef struct {
    uint32_t arena_id;
    uint32_t offset;
} cns_arena_handle_t;

// 7T string reference - O(1) lookup and comparison via interning
typedef struct {
    cns_hash_t hash;        // Precomputed hash for O(1) comparison
    uint32_t offset;        // Offset in string arena
    uint16_t length;        // String length (max 64KB)
    uint16_t ref_count;     // Reference counting for GC
} cns_string_ref_t;

// 7T memory region descriptor - deterministic memory contracts
typedef struct {
    void *ptr;              // Memory pointer (aligned to 64-byte boundary)
    size_t size;            // Region size
    size_t used;            // Used bytes
    uint32_t flags;         // Memory flags
    uint32_t magic;         // Memory corruption detection
} cns_memory_region_t;

// 7T performance metrics
typedef struct {
    cns_tick_t start_tick;
    cns_tick_t end_tick;
    uint32_t operation_count;
    uint32_t cache_hits;
    uint32_t cache_misses;
    size_t memory_allocated;
    size_t memory_freed;
} cns_perf_metrics_t;

// ============================================================================
// ERROR HANDLING - DETERMINISTIC AND AOT-FRIENDLY
// ============================================================================

// 7T error codes - designed for AOT compilation efficiency
typedef enum {
    CNS_OK = 0,
    CNS_ERROR_MEMORY = 1,
    CNS_ERROR_INVALID_ARG = 2,
    CNS_ERROR_NOT_FOUND = 3,
    CNS_ERROR_CAPACITY = 4,
    CNS_ERROR_TIMEOUT = 5,
    CNS_ERROR_CORRUPTION = 6,
    CNS_ERROR_7T_VIOLATION = 7,    // Operation exceeded 7-tick limit
    CNS_ERROR_ALIGNMENT = 8,       // Memory alignment violation
    CNS_ERROR_ARENA_FULL = 9,      // Arena allocator full
    CNS_ERROR_STRING_INTERN = 10,  // String interning failure
    CNS_ERROR_PARSER = 11,         // Parser error
    CNS_ERROR_SHACL = 12,          // SHACL validation error
    CNS_ERROR_MATERIALIZER = 13    // Binary materializer error
} cns_result_t;

// Error context for debugging - minimal overhead
typedef struct {
    cns_result_t code;
    uint32_t line;
    const char *file;
    const char *message;
    cns_tick_t error_tick;
} cns_error_context_t;

// ============================================================================
// FORWARD DECLARATIONS FOR 7T SUBSTRATE COMPONENTS
// ============================================================================

// Core substrate components
typedef struct cns_arena cns_arena_t;
typedef struct cns_interner cns_interner_t;
typedef struct cns_graph cns_graph_t;
typedef struct cns_parser cns_parser_t;
typedef struct cns_shacl_validator cns_shacl_validator_t;
typedef struct cns_materializer cns_materializer_t;

// Graph components
typedef struct cns_node cns_node_t;
typedef struct cns_edge cns_edge_t;
typedef struct cns_triple cns_triple_t;

// Parser components
typedef struct cns_token cns_token_t;
typedef struct cns_ast_node cns_ast_node_t;

// SHACL components
typedef struct cns_shape cns_shape_t;
typedef struct cns_constraint cns_constraint_t;
typedef struct cns_validation_result cns_validation_result_t;

// ============================================================================
// CALLBACK TYPES FOR 7T SUBSTRATE
// ============================================================================

// Memory allocation callback - must complete within 7 ticks
typedef void* (*cns_alloc_fn_t)(size_t size, void *user_data);
typedef void (*cns_free_fn_t)(void *ptr, void *user_data);

// Error handling callback
typedef void (*cns_error_fn_t)(const cns_error_context_t *error, void *user_data);

// Performance monitoring callback
typedef void (*cns_perf_fn_t)(const cns_perf_metrics_t *metrics, void *user_data);

// Graph traversal callback - O(1) per node
typedef cns_result_t (*cns_visit_fn_t)(cns_node_t *node, void *user_data);

// Parser callback for incremental parsing
typedef cns_result_t (*cns_parse_fn_t)(cns_token_t *token, void *user_data);

// SHACL validation callback
typedef cns_result_t (*cns_validate_fn_t)(cns_shape_t *shape, cns_node_t *target, void *user_data);

// ============================================================================
// 7T SUBSTRATE CONFIGURATION
// ============================================================================

// Arena allocator configuration
typedef struct {
    size_t initial_size;    // Initial arena size
    size_t max_size;        // Maximum arena size
    size_t alignment;       // Memory alignment (must be power of 2)
    bool enable_guard;      // Enable guard pages for debugging
    cns_alloc_fn_t alloc;   // Custom allocator
    cns_free_fn_t free;     // Custom deallocator
    void *user_data;        // User data for callbacks
} cns_arena_config_t;

// String interner configuration
typedef struct {
    size_t initial_capacity;  // Initial hash table capacity
    size_t string_arena_size; // Size of string storage arena
    float load_factor;        // Hash table load factor
    bool case_sensitive;      // Case sensitivity for strings
} cns_interner_config_t;

// Graph configuration
typedef struct {
    size_t initial_nodes;     // Initial node capacity
    size_t initial_edges;     // Initial edge capacity
    bool directed;            // Directed or undirected graph
    bool allow_self_loops;    // Allow self-loops
    bool allow_multi_edges;   // Allow multiple edges between nodes
    cns_arena_t *arena;       // Arena for graph memory
} cns_graph_config_t;

// Parser configuration
typedef struct {
    size_t buffer_size;       // Input buffer size
    size_t max_token_length;  // Maximum token length
    bool strict_mode;         // Strict parsing mode
    cns_arena_t *arena;       // Arena for AST nodes
    cns_parse_fn_t callback;  // Incremental parsing callback
    void *user_data;          // User data for callback
} cns_parser_config_t;

// SHACL validator configuration
typedef struct {
    size_t max_shapes;        // Maximum number of shapes
    size_t max_constraints;   // Maximum constraints per shape
    bool enable_closed;       // Enable closed shape validation
    bool enable_deactivated;  // Process deactivated shapes
    cns_arena_t *arena;       // Arena for validation data
} cns_shacl_config_t;

// Binary materializer configuration
typedef struct {
    size_t buffer_size;       // Serialization buffer size
    bool enable_compression;  // Enable compression
    bool enable_checksum;     // Enable checksum validation
    bool enable_zero_copy;    // Enable zero-copy deserialization
    cns_arena_t *arena;       // Arena for materialization
} cns_materializer_config_t;

// ============================================================================
// 7T SUBSTRATE CONTEXT - UNIFIED EXECUTION ENVIRONMENT
// ============================================================================

// Main 7T substrate context - coordinates all components
typedef struct {
    // Core allocators
    cns_arena_t *main_arena;     // Main memory arena
    cns_interner_t *interner;    // String interner
    
    // Graph storage
    cns_graph_t *graph;          // RDF graph
    
    // Processing components
    cns_parser_t *parser;        // TTL/RDF parser
    cns_shacl_validator_t *validator; // SHACL validator
    cns_materializer_t *materializer; // Binary materializer
    
    // Performance tracking
    cns_perf_metrics_t metrics;  // Performance metrics
    cns_tick_t operation_start;  // Current operation start tick
    
    // Error handling
    cns_error_fn_t error_handler; // Error callback
    cns_perf_fn_t perf_handler;   // Performance callback
    void *user_data;              // User data for callbacks
    
    // Configuration flags
    uint32_t flags;              // Configuration flags
    bool debug_mode;             // Debug mode enabled
    bool strict_7t;              // Strict 7-tick enforcement
} cns_context_t;

// Context flags
#define CNS_FLAG_ENABLE_PROFILING   (1 << 0)
#define CNS_FLAG_ENABLE_VALIDATION  (1 << 1)
#define CNS_FLAG_ENABLE_COMPRESSION (1 << 2)
#define CNS_FLAG_STRICT_7T          (1 << 3)
#define CNS_FLAG_DEBUG_MEMORY       (1 << 4)
#define CNS_FLAG_ZERO_COPY          (1 << 5)

// ============================================================================
// COMPATIBILITY WITH EXISTING CNS TYPES
// ============================================================================

// Command handler function type
typedef int (*CNSHandler)(CNSContext *ctx, int argc, char **argv);

// Option types
typedef enum {
    CNS_OPT_STRING,
    CNS_OPT_INT,
    CNS_OPT_BOOL,
    CNS_OPT_FLOAT,
    CNS_OPT_FLAG
} CNSOptionType;

// Forward declarations for CLI compatibility
typedef struct CNSContext CNSContext;
typedef struct CNSCommand CNSCommand;
typedef struct CNSDomain CNSDomain;
typedef struct CNSOption CNSOption;
typedef struct CNSArgument CNSArgument;

// Option structure
struct CNSOption {
    const char *name;        // Long name (e.g., "input")
    char short_name;         // Short name (e.g., 'i')
    CNSOptionType type;      // Option type
    const char *description; // Help text
    const char *default_val; // Default value
    bool required;           // Is required?
};

// Argument structure
struct CNSArgument {
    const char *name;        // Argument name
    const char *description; // Help text
    bool required;           // Is required?
    bool variadic;           // Accept multiple values?
};

// Command structure
struct CNSCommand {
    const char *name;        // Command name (verb)
    const char *description; // Help text
    CNSHandler handler;      // Function to execute
    CNSOption *options;      // Array of options
    size_t option_count;     // Number of options
    CNSArgument *arguments;  // Array of arguments
    size_t argument_count;   // Number of arguments
};

// Domain structure (groups related commands)
struct CNSDomain {
    const char *name;        // Domain name
    const char *description; // Help text
    CNSCommand *commands;    // Array of commands
    size_t command_count;    // Number of commands
};

// Context for command execution
struct CNSContext {
    const char *program_name; // Program name (cns)
    const char *domain;       // Current domain
    const char *command;      // Current command
    void *parsed_options;     // Parsed options
    void *parsed_arguments;   // Parsed arguments
    bool verbose;             // Verbose output
    bool json_output;         // JSON output mode
    uint64_t start_cycles;    // Performance tracking
    void *user_data;          // User-defined data
    
    // 7T substrate integration
    cns_context_t *substrate; // 7T substrate context
};

// Use the result codes from base.h
typedef cns_result_t CNSResult;

// ============================================================================
// UTILITY MACROS FOR 7T PERFORMANCE
// ============================================================================

// 7T tick measurement
#define CNS_7T_START_TIMING(ctx) \
    do { (ctx)->operation_start = cns_get_tick_count(); } while(0)

#define CNS_7T_END_TIMING(ctx) \
    do { \
        cns_tick_t elapsed = cns_get_tick_count() - (ctx)->operation_start; \
        if (elapsed > CNS_7T_TICK_LIMIT && (ctx)->strict_7t) { \
            cns_error_context_t error = { \
                .code = CNS_ERROR_7T_VIOLATION, \
                .line = __LINE__, \
                .file = __FILE__, \
                .message = "Operation exceeded 7-tick limit", \
                .error_tick = elapsed \
            }; \
            if ((ctx)->error_handler) (ctx)->error_handler(&error, (ctx)->user_data); \
        } \
    } while(0)

// Memory alignment macro
#define CNS_7T_ALIGN(size) (((size) + CNS_7T_ALIGNMENT - 1) & ~(CNS_7T_ALIGNMENT - 1))

// Cache line alignment
#define CNS_7T_CACHE_ALIGN(ptr) \
    ((void*)(((uintptr_t)(ptr) + CNS_7T_CACHE_LINE_SIZE - 1) & ~(CNS_7T_CACHE_LINE_SIZE - 1)))

// Branch prediction hints for AOT compilation
#ifdef __GNUC__
#define CNS_7T_LIKELY(x)   __builtin_expect(!!(x), 1)
#define CNS_7T_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define CNS_7T_LIKELY(x)   (x)
#define CNS_7T_UNLIKELY(x) (x)
#endif

// ============================================================================
// PLATFORM-SPECIFIC OPTIMIZATIONS
// ============================================================================

// Tick counter implementation
static inline cns_tick_t cns_get_tick_count(void) {
#if defined(__x86_64__) || defined(__i386__)
    uint64_t tsc;
    __asm__ volatile ("rdtsc" : "=A" (tsc));
    return tsc;
#elif defined(__aarch64__)
    uint64_t tsc;
    __asm__ volatile ("mrs %0, cntvct_el0" : "=r" (tsc));
    return tsc;
#else
    // Fallback to clock_gettime for portability
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#endif
}

#ifdef __cplusplus
}
#endif

#endif // CNS_TYPES_H