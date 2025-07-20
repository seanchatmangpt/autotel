/*  ─────────────────────────────────────────────────────────────
    cns_transpile.c  –  CNS AOT Transpiler Main Implementation
    Complete logic → proof → physical artifact pipeline (7T-compliant)
    ───────────────────────────────────────────────────────────── */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cns.h"
#include "cns/arena.h"
#include "cns/interner.h"
#include "cns/parser.h"
#include "cns/graph.h"
#include "cns/shacl.h"
#include "cns/binary_materializer.h"

/*═══════════════════════════════════════════════════════════════
  AOT Transpiler Configuration and Constants
  ═══════════════════════════════════════════════════════════════*/

#define CNS_TRANSPILE_VERSION_MAJOR 1
#define CNS_TRANSPILE_VERSION_MINOR 0
#define CNS_TRANSPILE_VERSION_PATCH 0

// Memory arena sizes (optimized for typical TTL files)
#define CNS_PARSER_ARENA_SIZE   (4 * 1024 * 1024)   // 4MB for parsing
#define CNS_GRAPH_ARENA_SIZE    (8 * 1024 * 1024)   // 8MB for graph storage
#define CNS_INTERNER_ARENA_SIZE (2 * 1024 * 1024)   // 2MB for string interning
#define CNS_SHACL_ARENA_SIZE    (2 * 1024 * 1024)   // 2MB for validation
#define CNS_BINARY_ARENA_SIZE   (4 * 1024 * 1024)   // 4MB for serialization

// Performance thresholds
#define CNS_MAX_PARSE_TICKS     (1000 * 7)  // 1000 operations * 7 ticks max
#define CNS_MAX_VALIDATE_TICKS  (500 * 7)   // 500 operations * 7 ticks max
#define CNS_MAX_SERIALIZE_TICKS (200 * 7)   // 200 operations * 7 ticks max

// File extensions
#define CNS_TTL_EXTENSION       ".ttl"
#define CNS_PLAN_EXTENSION      ".plan.bin"

/*═══════════════════════════════════════════════════════════════
  AOT Transpiler Context Structure
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    // Memory management
    arena_t *main_arena;          // Main arena for allocations
    arena_t *parser_arena;        // Arena for parser
    arena_t *graph_arena;         // Arena for graph
    arena_t *interner_arena;      // Arena for string interner
    arena_t *shacl_arena;         // Arena for SHACL validator
    arena_t *binary_arena;        // Arena for binary serialization
    
    // Core components
    cns_interner_t *interner;     // String interner
    cns_parser_t *parser;         // TTL parser
    cns_graph_t *graph;           // RDF graph
    cns_shacl_validator_t *validator; // SHACL validator
    
    // Input/Output
    const char *input_file;       // Input TTL file path
    const char *output_file;      // Output .plan.bin file path
    
    // Configuration
    bool validate_with_shacl;     // Enable SHACL validation
    bool strict_mode;             // Strict parsing/validation
    bool verbose_output;          // Verbose progress reporting
    bool show_stats;              // Show performance statistics
    bool debug_mode;              // Debug mode with detailed output
    
    // Performance tracking
    uint64_t start_time;          // Transpilation start time
    uint64_t parse_ticks;         // Parsing time in ticks
    uint64_t validate_ticks;      // Validation time in ticks
    uint64_t serialize_ticks;     // Serialization time in ticks
    uint64_t total_ticks;         // Total transpilation time
    
    // Statistics
    size_t input_size;            // Input file size in bytes
    size_t output_size;           // Output file size in bytes
    size_t triples_parsed;        // Number of triples parsed
    size_t validation_errors;     // Number of validation errors
    size_t memory_peak;           // Peak memory usage
    
    // Error handling
    char error_message[256];      // Last error message
    int error_code;               // Last error code
} cns_transpile_context_t;

/*═══════════════════════════════════════════════════════════════
  Utility Functions
  ═══════════════════════════════════════════════════════════════*/

// Get current time in nanoseconds
static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// Convert nanoseconds to CPU ticks (approximate)
static uint64_t ns_to_ticks(uint64_t ns) {
    // Assume 2.5GHz CPU for tick conversion
    return (ns * 25) / 10;
}

// Get file size
static size_t get_file_size(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        return st.st_size;
    }
    return 0;
}

// Check if file exists
static bool file_exists(const char *filename) {
    return access(filename, F_OK) == 0;
}

// Generate output filename from input filename
static char* generate_output_filename(const char *input_file, arena_t *arena) {
    size_t input_len = strlen(input_file);
    const char *dot = strrchr(input_file, '.');
    
    size_t base_len;
    if (dot && strcmp(dot, CNS_TTL_EXTENSION) == 0) {
        base_len = dot - input_file;
    } else {
        base_len = input_len;
    }
    
    size_t output_len = base_len + strlen(CNS_PLAN_EXTENSION) + 1;
    char *output_file = arenac_alloc(arena, output_len);
    if (!output_file) {
        return NULL;
    }
    
    memcpy(output_file, input_file, base_len);
    strcpy(output_file + base_len, CNS_PLAN_EXTENSION);
    
    return output_file;
}

/*═══════════════════════════════════════════════════════════════
  Context Management Functions
  ═══════════════════════════════════════════════════════════════*/

// Initialize transpiler context
static int cns_transpile_init_context(cns_transpile_context_t *ctx) {
    memset(ctx, 0, sizeof(*ctx));
    
    // Create main arena for context allocations
    ctx->main_arena = arenac_create(1024 * 1024, ARENAC_FLAG_ZERO_ALLOC);
    if (!ctx->main_arena) {
        snprintf(ctx->error_message, sizeof(ctx->error_message), 
                "Failed to create main arena");
        return -1;
    }
    
    // Create specialized arenas for each component
    ctx->parser_arena = arenac_create(CNS_PARSER_ARENA_SIZE, ARENAC_FLAG_ZERO_ALLOC);
    ctx->graph_arena = arenac_create(CNS_GRAPH_ARENA_SIZE, ARENAC_FLAG_ZERO_ALLOC);
    ctx->interner_arena = arenac_create(CNS_INTERNER_ARENA_SIZE, ARENAC_FLAG_ZERO_ALLOC);
    ctx->shacl_arena = arenac_create(CNS_SHACL_ARENA_SIZE, ARENAC_FLAG_ZERO_ALLOC);
    ctx->binary_arena = arenac_create(CNS_BINARY_ARENA_SIZE, ARENAC_FLAG_ZERO_ALLOC);
    
    if (!ctx->parser_arena || !ctx->graph_arena || !ctx->interner_arena || 
        !ctx->shacl_arena || !ctx->binary_arena) {
        snprintf(ctx->error_message, sizeof(ctx->error_message), 
                "Failed to create component arenas");
        return -1;
    }
    
    return 0;
}

// Initialize core components
static int cns_transpile_init_components(cns_transpile_context_t *ctx) {
    // Initialize string interner
    ctx->interner = cns_interner_create_default(ctx->interner_arena);
    if (!ctx->interner) {
        snprintf(ctx->error_message, sizeof(ctx->error_message), 
                "Failed to create string interner");
        return -1;
    }
    
    // Initialize RDF graph
    ctx->graph = cns_graph_create_default(ctx->graph_arena, ctx->interner);
    if (!ctx->graph) {
        snprintf(ctx->error_message, sizeof(ctx->error_message), 
                "Failed to create RDF graph");
        return -1;
    }
    
    // Initialize TTL parser
    ctx->parser = cns_parser_create_default(ctx->parser_arena, ctx->interner, ctx->graph);
    if (!ctx->parser) {
        snprintf(ctx->error_message, sizeof(ctx->error_message), 
                "Failed to create TTL parser");
        return -1;
    }
    
    // Initialize SHACL validator if enabled
    if (ctx->validate_with_shacl) {
        ctx->validator = cns_shacl_validator_create_default(ctx->shacl_arena, ctx->interner);
        if (!ctx->validator) {
            snprintf(ctx->error_message, sizeof(ctx->error_message), 
                    "Failed to create SHACL validator");
            return -1;
        }
    }
    
    // Configure components based on options
    if (ctx->strict_mode) {
        cns_parser_set_flags(ctx->parser, 
            CNS_PARSER_FLAG_STRICT | CNS_PARSER_FLAG_VALIDATE_IRIS);
        
        if (ctx->validator) {
            cns_shacl_set_flags(ctx->validator, CNS_SHACL_FLAG_STRICT_MODE);
        }
    }
    
    return 0;
}

// Cleanup transpiler context
static void cns_transpile_cleanup_context(cns_transpile_context_t *ctx) {
    if (ctx->parser) {
        cns_parser_destroy(ctx->parser);
    }
    if (ctx->graph) {
        cns_graph_destroy(ctx->graph);
    }
    if (ctx->interner) {
        cns_interner_destroy(ctx->interner);
    }
    if (ctx->validator) {
        cns_shacl_validator_destroy(ctx->validator);
    }
    
    // Destroy arenas (automatically frees all allocations)
    if (ctx->parser_arena) arenac_destroy(ctx->parser_arena);
    if (ctx->graph_arena) arenac_destroy(ctx->graph_arena);
    if (ctx->interner_arena) arenac_destroy(ctx->interner_arena);
    if (ctx->shacl_arena) arenac_destroy(ctx->shacl_arena);
    if (ctx->binary_arena) arenac_destroy(ctx->binary_arena);
    if (ctx->main_arena) arenac_destroy(ctx->main_arena);
    
    memset(ctx, 0, sizeof(*ctx));
}

/*═══════════════════════════════════════════════════════════════
  Pipeline Implementation Functions
  ═══════════════════════════════════════════════════════════════*/

// Parse TTL input file
static int cns_transpile_parse(cns_transpile_context_t *ctx) {
    if (ctx->verbose_output) {
        printf("Parsing TTL file: %s\n", ctx->input_file);
    }
    
    uint64_t start_time = get_time_ns();
    
    // Parse the TTL file
    cns_result_t result = cns_parser_parse_file(ctx->parser, ctx->input_file);
    
    uint64_t end_time = get_time_ns();
    ctx->parse_ticks = ns_to_ticks(end_time - start_time);
    
    if (result != CNS_OK) {
        snprintf(ctx->error_message, sizeof(ctx->error_message), 
                "Failed to parse TTL file: error code %d", result);
        return -1;
    }
    
    // Check performance constraint
    if (ctx->parse_ticks > CNS_MAX_PARSE_TICKS) {
        if (ctx->debug_mode) {
            printf("Warning: Parsing exceeded 7T performance constraint (%lu ticks)\n", 
                   ctx->parse_ticks);
        }
    }
    
    // Get parsing statistics
    cns_parser_stats_t parser_stats;
    cns_parser_get_stats(ctx->parser, &parser_stats);
    ctx->triples_parsed = parser_stats.triples_parsed;
    
    if (ctx->verbose_output) {
        printf("Parsed %zu triples in %lu ticks\n", 
               ctx->triples_parsed, ctx->parse_ticks);
    }
    
    return 0;
}

// Validate graph with SHACL
static int cns_transpile_validate(cns_transpile_context_t *ctx) {
    if (!ctx->validate_with_shacl || !ctx->validator) {
        return 0;  // Validation disabled
    }
    
    if (ctx->verbose_output) {
        printf("Validating graph with SHACL constraints\n");
    }
    
    uint64_t start_time = get_time_ns();
    
    // Create validation report
    cns_validation_report_t *report = cns_shacl_create_report(ctx->validator);
    if (!report) {
        snprintf(ctx->error_message, sizeof(ctx->error_message), 
                "Failed to create validation report");
        return -1;
    }
    
    // Validate the graph
    cns_result_t result = cns_shacl_validate_graph(ctx->validator, ctx->graph, report);
    
    uint64_t end_time = get_time_ns();
    ctx->validate_ticks = ns_to_ticks(end_time - start_time);
    
    if (result != CNS_OK) {
        snprintf(ctx->error_message, sizeof(ctx->error_message), 
                "SHACL validation failed: error code %d", result);
        cns_shacl_destroy_report(report);
        return -1;
    }
    
    // Check validation results
    ctx->validation_errors = report->violation_count;
    
    if (ctx->verbose_output || !report->conforms) {
        printf("Validation results: %s\n", report->conforms ? "CONFORMS" : "VIOLATIONS");
        printf("  Violations: %zu\n", report->violation_count);
        printf("  Warnings: %zu\n", report->warning_count);
        printf("  Info: %zu\n", report->info_count);
    }
    
    // In strict mode, violations are errors
    if (ctx->strict_mode && !report->conforms) {
        snprintf(ctx->error_message, sizeof(ctx->error_message), 
                "Graph does not conform to SHACL constraints (%zu violations)", 
                report->violation_count);
        cns_shacl_destroy_report(report);
        return -1;
    }
    
    cns_shacl_destroy_report(report);
    
    if (ctx->verbose_output) {
        printf("Validation completed in %lu ticks\n", ctx->validate_ticks);
    }
    
    return 0;
}

// Serialize graph to binary format
static int cns_transpile_serialize(cns_transpile_context_t *ctx) {
    if (ctx->verbose_output) {
        printf("Serializing graph to binary format: %s\n", ctx->output_file);
    }
    
    uint64_t start_time = get_time_ns();
    
    // Create write buffer
    cns_write_buffer_t *buffer = cns_write_buffer_create(ctx->binary_arena->size / 2);
    if (!buffer) {
        snprintf(ctx->error_message, sizeof(ctx->error_message), 
                "Failed to create write buffer");
        return -1;
    }
    
    // Serialize graph to buffer
    int result = cns_graph_serialize(ctx->graph, buffer, 
                                   CNS_SERIALIZE_FLAG_COMPRESS | CNS_SERIALIZE_FLAG_CHECKSUM);
    
    if (result != CNS_SERIALIZE_OK) {
        snprintf(ctx->error_message, sizeof(ctx->error_message), 
                "Failed to serialize graph: %s", cns_serialize_error_string(result));
        cns_write_buffer_destroy(buffer);
        return -1;
    }
    
    // Write buffer to file
    FILE *output_fp = fopen(ctx->output_file, "wb");
    if (!output_fp) {
        snprintf(ctx->error_message, sizeof(ctx->error_message), 
                "Failed to open output file: %s", strerror(errno));
        cns_write_buffer_destroy(buffer);
        return -1;
    }
    
    size_t written = fwrite(buffer->data, 1, buffer->size, output_fp);
    fclose(output_fp);
    
    if (written != buffer->size) {
        snprintf(ctx->error_message, sizeof(ctx->error_message), 
                "Failed to write complete output file");
        cns_write_buffer_destroy(buffer);
        return -1;
    }
    
    ctx->output_size = buffer->size;
    cns_write_buffer_destroy(buffer);
    
    uint64_t end_time = get_time_ns();
    ctx->serialize_ticks = ns_to_ticks(end_time - start_time);
    
    if (ctx->verbose_output) {
        printf("Serialized %zu bytes in %lu ticks\n", 
               ctx->output_size, ctx->serialize_ticks);
    }
    
    return 0;
}

/*═══════════════════════════════════════════════════════════════
  Main Transpilation Function
  ═══════════════════════════════════════════════════════════════*/

static int cns_transpile_execute(cns_transpile_context_t *ctx) {
    ctx->start_time = get_time_ns();
    
    // Validate input file exists
    if (!file_exists(ctx->input_file)) {
        snprintf(ctx->error_message, sizeof(ctx->error_message), 
                "Input file does not exist: %s", ctx->input_file);
        return -1;
    }
    
    ctx->input_size = get_file_size(ctx->input_file);
    
    // Generate output file name if not provided
    if (!ctx->output_file) {
        ctx->output_file = generate_output_filename(ctx->input_file, ctx->main_arena);
        if (!ctx->output_file) {
            snprintf(ctx->error_message, sizeof(ctx->error_message), 
                    "Failed to generate output filename");
            return -1;
        }
    }
    
    // Initialize components
    if (cns_transpile_init_components(ctx) != 0) {
        return -1;
    }
    
    // Execute pipeline: Parse → Validate → Serialize
    if (cns_transpile_parse(ctx) != 0) {
        return -1;
    }
    
    if (cns_transpile_validate(ctx) != 0) {
        return -1;
    }
    
    if (cns_transpile_serialize(ctx) != 0) {
        return -1;
    }
    
    // Calculate total time
    uint64_t end_time = get_time_ns();
    ctx->total_ticks = ns_to_ticks(end_time - ctx->start_time);
    
    // Update memory peak usage
    arenac_info_t info;
    arenac_get_info(ctx->main_arena, &info);
    ctx->memory_peak = info.high_water_mark;
    
    return 0;
}

/*═══════════════════════════════════════════════════════════════
  Statistics and Reporting Functions
  ═══════════════════════════════════════════════════════════════*/

static void cns_transpile_print_stats(const cns_transpile_context_t *ctx) {
    printf("\n=== CNS AOT Transpiler Statistics ===\n");
    printf("Input file:      %s (%zu bytes)\n", ctx->input_file, ctx->input_size);
    printf("Output file:     %s (%zu bytes)\n", ctx->output_file, ctx->output_size);
    printf("Compression:     %.1f%% (%.1fx)\n", 
           100.0 * (1.0 - (double)ctx->output_size / ctx->input_size),
           (double)ctx->input_size / ctx->output_size);
    printf("\nPerformance:\n");
    printf("  Parse time:    %lu ticks (%.2f ms)\n", 
           ctx->parse_ticks, ctx->parse_ticks / 2500000.0);
    printf("  Validate time: %lu ticks (%.2f ms)\n", 
           ctx->validate_ticks, ctx->validate_ticks / 2500000.0);
    printf("  Serialize time:%lu ticks (%.2f ms)\n", 
           ctx->serialize_ticks, ctx->serialize_ticks / 2500000.0);
    printf("  Total time:    %lu ticks (%.2f ms)\n", 
           ctx->total_ticks, ctx->total_ticks / 2500000.0);
    printf("\nData:\n");
    printf("  Triples parsed:%zu\n", ctx->triples_parsed);
    printf("  Validation errors: %zu\n", ctx->validation_errors);
    printf("  Peak memory:   %zu KB\n", ctx->memory_peak / 1024);
    printf("  Throughput:    %.0f triples/sec\n", 
           ctx->triples_parsed * 1000000000.0 / (ctx->total_ticks * 400.0));
    
    // Check 7T compliance
    bool parse_compliant = ctx->parse_ticks <= CNS_MAX_PARSE_TICKS;
    bool validate_compliant = ctx->validate_ticks <= CNS_MAX_VALIDATE_TICKS;
    bool serialize_compliant = ctx->serialize_ticks <= CNS_MAX_SERIALIZE_TICKS;
    
    printf("\n7T Compliance:\n");
    printf("  Parsing:       %s\n", parse_compliant ? "✓ COMPLIANT" : "✗ EXCEEDED");
    printf("  Validation:    %s\n", validate_compliant ? "✓ COMPLIANT" : "✗ EXCEEDED");
    printf("  Serialization: %s\n", serialize_compliant ? "✓ COMPLIANT" : "✗ EXCEEDED");
    printf("  Overall:       %s\n", 
           (parse_compliant && validate_compliant && serialize_compliant) ? 
           "✓ 7T COMPLIANT" : "✗ 7T EXCEEDED");
}

/*═══════════════════════════════════════════════════════════════
  Command Line Interface
  ═══════════════════════════════════════════════════════════════*/

static void print_usage(const char *program_name) {
    printf("CNS AOT Transpiler v%d.%d.%d\n", 
           CNS_TRANSPILE_VERSION_MAJOR, CNS_TRANSPILE_VERSION_MINOR, CNS_TRANSPILE_VERSION_PATCH);
    printf("Complete logic → proof → physical artifact pipeline\n\n");
    printf("Usage: %s [OPTIONS] INPUT_FILE [OUTPUT_FILE]\n\n", program_name);
    printf("Arguments:\n");
    printf("  INPUT_FILE     Input TTL file to transpile\n");
    printf("  OUTPUT_FILE    Output .plan.bin file (optional, auto-generated if not provided)\n\n");
    printf("Options:\n");
    printf("  -s, --strict       Enable strict mode (validation errors are fatal)\n");
    printf("  -v, --verbose      Enable verbose output\n");
    printf("  -S, --stats        Show detailed statistics\n");
    printf("  -d, --debug        Enable debug mode\n");
    printf("  --no-validate     Disable SHACL validation\n");
    printf("  -h, --help        Show this help message\n");
    printf("  --version         Show version information\n\n");
    printf("Examples:\n");
    printf("  %s ontology.ttl                    # Basic transpilation\n", program_name);
    printf("  %s --strict --verbose data.ttl     # Strict mode with verbose output\n", program_name);
    printf("  %s input.ttl output.plan.bin       # Specify output file\n", program_name);
}

static void print_version(void) {
    printf("CNS AOT Transpiler %d.%d.%d\n", 
           CNS_TRANSPILE_VERSION_MAJOR, CNS_TRANSPILE_VERSION_MINOR, CNS_TRANSPILE_VERSION_PATCH);
    printf("Built with 7T substrate for deterministic performance\n");
    printf("Components: ARENAC, Parser, Graph, SHACL, Binary Materializer\n");
}

/*═══════════════════════════════════════════════════════════════
  Main Function
  ═══════════════════════════════════════════════════════════════*/

int main(int argc, char *argv[]) {
    cns_transpile_context_t ctx;
    int result = 0;
    
    // Initialize context
    if (cns_transpile_init_context(&ctx) != 0) {
        fprintf(stderr, "Error: %s\n", ctx.error_message);
        return 1;
    }
    
    // Set default options
    ctx.validate_with_shacl = true;
    ctx.strict_mode = false;
    ctx.verbose_output = false;
    ctx.show_stats = false;
    ctx.debug_mode = false;
    
    // Parse command line arguments
    int arg_idx = 1;
    while (arg_idx < argc) {
        const char *arg = argv[arg_idx];
        
        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            print_usage(argv[0]);
            cns_transpile_cleanup_context(&ctx);
            return 0;
        } else if (strcmp(arg, "--version") == 0) {
            print_version();
            cns_transpile_cleanup_context(&ctx);
            return 0;
        } else if (strcmp(arg, "-s") == 0 || strcmp(arg, "--strict") == 0) {
            ctx.strict_mode = true;
        } else if (strcmp(arg, "-v") == 0 || strcmp(arg, "--verbose") == 0) {
            ctx.verbose_output = true;
        } else if (strcmp(arg, "-S") == 0 || strcmp(arg, "--stats") == 0) {
            ctx.show_stats = true;
        } else if (strcmp(arg, "-d") == 0 || strcmp(arg, "--debug") == 0) {
            ctx.debug_mode = true;
            ctx.verbose_output = true;  // Debug implies verbose
        } else if (strcmp(arg, "--no-validate") == 0) {
            ctx.validate_with_shacl = false;
        } else if (arg[0] == '-') {
            fprintf(stderr, "Error: Unknown option '%s'\n", arg);
            print_usage(argv[0]);
            cns_transpile_cleanup_context(&ctx);
            return 1;
        } else {
            // Positional arguments
            if (!ctx.input_file) {
                ctx.input_file = arg;
            } else if (!ctx.output_file) {
                ctx.output_file = arg;
            } else {
                fprintf(stderr, "Error: Too many arguments\n");
                print_usage(argv[0]);
                cns_transpile_cleanup_context(&ctx);
                return 1;
            }
        }
        arg_idx++;
    }
    
    // Check required arguments
    if (!ctx.input_file) {
        fprintf(stderr, "Error: Input file is required\n");
        print_usage(argv[0]);
        cns_transpile_cleanup_context(&ctx);
        return 1;
    }
    
    // Execute transpilation
    if (cns_transpile_execute(&ctx) != 0) {
        fprintf(stderr, "Error: %s\n", ctx.error_message);
        result = 1;
    } else {
        if (ctx.verbose_output) {
            printf("Transpilation completed successfully\n");
        }
        
        if (ctx.show_stats || ctx.debug_mode) {
            cns_transpile_print_stats(&ctx);
        }
    }
    
    // Cleanup and exit
    cns_transpile_cleanup_context(&ctx);
    return result;
}