/**
 * @file cjinja_aot_domain.c
 * @brief CJinja AOT Domain Implementation - Uses compiled templates for 7-tick performance
 * 
 * This domain provides the interface to AOT-compiled Jinja templates.
 * Templates are compiled at build time for maximum performance.
 */

#include "cns/cli.h"
#include "cns/engines/cjinja_aot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>

// Include generated headers (will be available after AOT compilation)
#ifdef CNS_AOT_GENERATED
#include "cjinja_templates.h"
#include "cjinja_dispatcher.h"
#include "cjinja_context_parser.h"
#endif

// Performance measurement utilities
static inline uint64_t get_cycles(void) {
    return __builtin_readcyclecounter();
}

/**
 * @brief Render an AOT-compiled template with JSON context
 * @param ctx CNS context
 * @param argc Argument count
 * @param argv Arguments: [template_name] [context.json] [output_file?]
 * @return CNS result code
 */
static int cmd_cjinja_aot_render(CNSContext *ctx, int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: cns cjinja-aot render <template_name> <context.json> [output_file]\n");
        printf("\nAvailable templates:\n");
        
#ifdef CNS_AOT_GENERATED
        const char** templates = cjinja_list_templates();
        for (int i = 0; templates && templates[i]; i++) {
            printf("  - %s\n", templates[i]);
        }
#else
        printf("  (No AOT-compiled templates available - run 'make aot' first)\n");
#endif
        return CNS_ERR_INVALID_ARG;
    }
    
    const char* template_name = argv[0];
    const char* context_file = argv[1];
    const char* output_file = (argc > 2) ? argv[2] : NULL;
    
#ifndef CNS_AOT_GENERATED
    printf("❌ AOT-compiled templates not available.\n");
    printf("   Run 'make aot' to compile templates first.\n");
    return CNS_ERR_NOT_IMPLEMENTED;
#else
    
    // Check if template exists
    if (!cjinja_template_exists(template_name)) {
        printf("❌ Template '%s' not found.\n", template_name);
        printf("Available templates:\n");
        const char** templates = cjinja_list_templates();
        for (int i = 0; templates && templates[i]; i++) {
            printf("  - %s\n", templates[i]);
        }
        return CNS_ERR_NOT_FOUND;
    }
    
    // Allocate context structure
    void* context = cjinja_alloc_context(template_name);
    if (!context) {
        printf("❌ Failed to allocate context for template '%s'\n", template_name);
        return CNS_ERR_MEMORY;
    }
    
    // Load JSON context from file
    if (!cjinja_load_context_from_file(template_name, context_file, context)) {
        printf("❌ Failed to parse context file '%s'\n", context_file);
        cjinja_free_context(context);
        return CNS_ERR_INVALID_ARG;
    }
    
    // Prepare output buffer
    char output_buffer[16384]; // 16KB buffer
    
    // Render template with performance measurement
    cjinja_perf_metrics_t metrics;
    uint64_t start_cycles = get_cycles();
    
    int bytes_written = cjinja_render_with_metrics(
        template_name, context, output_buffer, sizeof(output_buffer), &metrics
    );
    
    uint64_t total_cycles = get_cycles() - start_cycles;
    
    if (bytes_written < 0) {
        printf("❌ Template rendering failed: %s\n", 
               cjinja_error_string((cjinja_result_t)bytes_written));
        cjinja_free_context(context);
        return CNS_ERR_RUNTIME;
    }
    
    // Output results
    if (output_file) {
        FILE* fp = fopen(output_file, "w");
        if (fp) {
            fwrite(output_buffer, 1, bytes_written, fp);
            fclose(fp);
            printf("✅ Template rendered to '%s'\n", output_file);
        } else {
            printf("❌ Failed to write to '%s'\n", output_file);
            cjinja_free_context(context);
            return CNS_ERR_IO;
        }
    } else {
        printf("---\n%.*s\n---\n", bytes_written, output_buffer);
    }
    
    // Performance report
    printf("\n📊 Performance Metrics:\n");
    printf("   Template: %s\n", template_name);
    printf("   Render cycles: %lu\n", metrics.render_cycles);
    printf("   Total cycles: %lu\n", total_cycles);
    printf("   Bytes written: %zu\n", metrics.bytes_written);
    
    if (metrics.exceeded_7tick_limit) {
        printf("   ⚠️  7-TICK VIOLATION: Rendering took %lu cycles (>7)\n", 
               metrics.render_cycles);
    } else {
        printf("   ✅ 7-TICK COMPLIANT: Rendering completed in %lu cycles\n", 
               metrics.render_cycles);
    }
    
    cjinja_free_context(context);
    return CNS_OK;
    
#endif
}

/**
 * @brief List all available AOT-compiled templates
 */
static int cmd_cjinja_aot_list(CNSContext *ctx, int argc, char **argv) {
    (void)ctx; (void)argc; (void)argv;
    
#ifndef CNS_AOT_GENERATED
    printf("❌ AOT-compiled templates not available.\n");
    printf("   Run 'make aot' to compile templates first.\n");
    return CNS_ERR_NOT_IMPLEMENTED;
#else
    
    printf("📋 Available AOT-Compiled Templates:\n\n");
    
    const char** templates = cjinja_list_templates();
    size_t template_count = cjinja_get_template_count();
    
    if (template_count == 0) {
        printf("   (No templates found)\n");
        return CNS_OK;
    }
    
    for (size_t i = 0; i < template_count && templates[i]; i++) {
        const cjinja_template_info_t* info = cjinja_get_template_info(templates[i]);
        
        printf("🔹 %s\n", templates[i]);
        if (info) {
            printf("   Source: %s\n", info->source_file);
            printf("   Context size: %zu bytes\n", info->context_size);
            printf("   Complexity: %d\n", info->complexity_score);
            printf("   Features: %s%s\n", 
                   info->has_loops ? "loops " : "",
                   info->has_conditionals ? "conditionals" : "");
        }
        printf("\n");
    }
    
    printf("Total: %zu templates\n", template_count);
    return CNS_OK;
    
#endif
}

/**
 * @brief Benchmark all AOT-compiled templates
 */
static int cmd_cjinja_aot_benchmark(CNSContext *ctx, int argc, char **argv) {
    (void)ctx;
    
    int iterations = 1000;
    if (argc > 0) {
        iterations = atoi(argv[0]);
        if (iterations <= 0) iterations = 1000;
    }
    
#ifndef CNS_AOT_GENERATED
    printf("❌ AOT-compiled templates not available.\n");
    printf("   Run 'make aot' to compile templates first.\n");
    return CNS_ERR_NOT_IMPLEMENTED;
#else
    
    printf("🚀 Benchmarking AOT-Compiled Templates (%d iterations each)\n\n", iterations);
    
    int failed_templates = cjinja_benchmark_all_templates(iterations, true);
    
    if (failed_templates == 0) {
        printf("\n🎉 All templates are 7-tick compliant!\n");
    } else {
        printf("\n⚠️  %d templates failed 7-tick compliance\n", failed_templates);
    }
    
    return (failed_templates == 0) ? CNS_OK : CNS_ERR_PERFORMANCE;
    
#endif
}

/**
 * @brief Show detailed information about a specific template
 */
static int cmd_cjinja_aot_info(CNSContext *ctx, int argc, char **argv) {
    (void)ctx;
    
    if (argc < 1) {
        printf("Usage: cns cjinja-aot info <template_name>\n");
        return CNS_ERR_INVALID_ARG;
    }
    
    const char* template_name = argv[0];
    
#ifndef CNS_AOT_GENERATED
    printf("❌ AOT-compiled templates not available.\n");
    return CNS_ERR_NOT_IMPLEMENTED;
#else
    
    if (!cjinja_template_exists(template_name)) {
        printf("❌ Template '%s' not found.\n", template_name);
        return CNS_ERR_NOT_FOUND;
    }
    
    const cjinja_template_info_t* info = cjinja_get_template_info(template_name);
    if (!info) {
        printf("❌ Could not retrieve information for template '%s'\n", template_name);
        return CNS_ERR_RUNTIME;
    }
    
    printf("📄 Template Information: %s\n", template_name);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("Source File:      %s\n", info->source_file);
    printf("Context Size:     %zu bytes\n", info->context_size);
    printf("Complexity Score: %d\n", info->complexity_score);
    printf("Has Loops:        %s\n", info->has_loops ? "Yes" : "No");
    printf("Has Conditionals: %s\n", info->has_conditionals ? "Yes" : "No");
    printf("Compiled:         %lu (unix timestamp)\n", info->compile_timestamp);
    printf("\n");
    
    // Show detailed statistics
    cjinja_print_template_stats(template_name);
    
    return CNS_OK;
    
#endif
}

/**
 * @brief Validate 7-tick compliance for a specific template
 */
static int cmd_cjinja_aot_validate(CNSContext *ctx, int argc, char **argv) {
    (void)ctx;
    
    if (argc < 1) {
        printf("Usage: cns cjinja-aot validate <template_name> [iterations]\n");
        return CNS_ERR_INVALID_ARG;
    }
    
    const char* template_name = argv[0];
    int iterations = (argc > 1) ? atoi(argv[1]) : 100;
    if (iterations <= 0) iterations = 100;
    
#ifndef CNS_AOT_GENERATED
    printf("❌ AOT-compiled templates not available.\n");
    return CNS_ERR_NOT_IMPLEMENTED;
#else
    
    if (!cjinja_template_exists(template_name)) {
        printf("❌ Template '%s' not found.\n", template_name);
        return CNS_ERR_NOT_FOUND;
    }
    
    // Create minimal test context
    void* context = cjinja_alloc_context(template_name);
    if (!context) {
        printf("❌ Failed to allocate context\n");
        return CNS_ERR_MEMORY;
    }
    
    printf("🔍 Validating 7-tick compliance for '%s' (%d iterations)\n", 
           template_name, iterations);
    
    bool is_compliant = cjinja_validate_7tick_compliance(template_name, context, iterations);
    
    if (is_compliant) {
        printf("✅ Template '%s' is 7-tick compliant\n", template_name);
    } else {
        printf("❌ Template '%s' violates 7-tick limit\n", template_name);
    }
    
    cjinja_free_context(context);
    return is_compliant ? CNS_OK : CNS_ERR_PERFORMANCE;
    
#endif
}

// Command definitions
static CNSCommand cjinja_aot_commands[] = {
    {
        .name = "render",
        .description = "Render AOT-compiled template with JSON context",
        .handler = cmd_cjinja_aot_render,
        .min_args = 2,
        .max_args = 3
    },
    {
        .name = "list",
        .description = "List all available AOT-compiled templates",
        .handler = cmd_cjinja_aot_list,
        .min_args = 0,
        .max_args = 0
    },
    {
        .name = "benchmark",
        .description = "Benchmark all AOT-compiled templates",
        .handler = cmd_cjinja_aot_benchmark,
        .min_args = 0,
        .max_args = 1
    },
    {
        .name = "info",
        .description = "Show detailed information about a template",
        .handler = cmd_cjinja_aot_info,
        .min_args = 1,
        .max_args = 1
    },
    {
        .name = "validate",
        .description = "Validate 7-tick compliance for a template",
        .handler = cmd_cjinja_aot_validate,
        .min_args = 1,
        .max_args = 2
    }
};

// Domain definition
CNSDomain cns_cjinja_aot_domain = {
    .name = "cjinja-aot",
    .description = "AOT-compiled Jinja template engine with 7-tick performance",
    .commands = cjinja_aot_commands,
    .command_count = sizeof(cjinja_aot_commands) / sizeof(CNSCommand)
};