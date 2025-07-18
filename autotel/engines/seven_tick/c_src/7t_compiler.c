#include "7t_compiler.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

struct SevenTickCompiler
{
    CompilerConfig config;
    FILE *output;
    char *code_buffer;
    size_t code_size;
    size_t code_capacity;
};

// Forward declarations
static void generate_l1_kernel(SevenTickCompiler *compiler, const SchemaDefinition *schema);
static void generate_l2_kernel(SevenTickCompiler *compiler, const SchemaDefinition *schema);
static void generate_l3_kernel(SevenTickCompiler *compiler, const SchemaDefinition *schema);
static void generate_sharded_kernel(SevenTickCompiler *compiler, const SchemaDefinition *schema);

// Create compiler instance
SevenTickCompiler *compiler_create(const CompilerConfig *config)
{
    SevenTickCompiler *compiler = calloc(1, sizeof(SevenTickCompiler));
    compiler->config = *config;
    compiler->code_capacity = 1024 * 1024; // 1MB initial
    compiler->code_buffer = malloc(compiler->code_capacity);
    return compiler;
}

// Emit code to buffer
static void emit_code(SevenTickCompiler *compiler, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    size_t remaining = compiler->code_capacity - compiler->code_size;
    int written = vsnprintf(compiler->code_buffer + compiler->code_size,
                            remaining, format, args);

    if (written > 0)
    {
        compiler->code_size += written;
    }

    va_end(args);
}

// Analyze schema to predict footprint
int compiler_analyze_schema(SevenTickCompiler *compiler,
                            const SchemaDefinition *schema,
                            KernelFootprint *footprint_out)
{
    memset(footprint_out, 0, sizeof(KernelFootprint));

    // Calculate OWL footprint
    size_t owl_size = calculate_owl_footprint(schema->num_classes,
                                              schema->num_properties);
    footprint_out->owl_class_masks_kb = owl_size * 0.8;
    footprint_out->owl_property_vectors_kb = owl_size * 0.2;

    // Calculate SHACL footprint
    size_t shacl_size = calculate_shacl_footprint(schema->num_shapes,
                                                  schema->num_properties);
    footprint_out->shacl_shape_masks_kb = shacl_size * 0.6;
    footprint_out->shacl_property_masks_kb = shacl_size * 0.4;

    // Calculate SPARQL footprint
    int use_index = (schema->expected_subjects < 10000); // Use index for small datasets
    size_t sparql_size = calculate_sparql_footprint(
        schema->expected_subjects,
        schema->expected_predicates,
        schema->expected_objects,
        use_index);

    if (use_index)
    {
        footprint_out->sparql_index_kb = sparql_size * 0.9;
        footprint_out->sparql_predicate_vectors_kb = sparql_size * 0.05;
        footprint_out->sparql_object_vectors_kb = sparql_size * 0.05;
    }
    else
    {
        footprint_out->sparql_predicate_vectors_kb = sparql_size * 0.3;
        footprint_out->sparql_object_vectors_kb = sparql_size * 0.7;
    }

    footprint_out->kernel_code_kb = 12; // Estimated
    footprint_out->total_footprint_kb =
        footprint_out->owl_class_masks_kb +
        footprint_out->owl_property_vectors_kb +
        footprint_out->shacl_shape_masks_kb +
        footprint_out->shacl_property_masks_kb +
        footprint_out->sparql_predicate_vectors_kb +
        footprint_out->sparql_object_vectors_kb +
        footprint_out->sparql_index_kb +
        footprint_out->kernel_code_kb;

    // Check if it fits in target tier
    ComplianceTier achievable_tier = calculate_compliance_tier(footprint_out);
    return (achievable_tier <= compiler->config.target_tier) ? 0 : -1;
}
// Generate optimized C code for the kernel
static void generate_kernel_code(SevenTickCompiler *compiler,
                                 const SchemaDefinition *schema,
                                 const KernelFootprint *footprint)
{
    compiler->code_size = 0; // Reset buffer

    // Header
    emit_code(compiler, "/* 7T Tier-Compliant Kernel - Auto-generated */\n");
    emit_code(compiler, "#include <stdint.h>\n");
    emit_code(compiler, "#include <string.h>\n\n");

    // Determine data structure strategy based on tier
    ComplianceTier tier = calculate_compliance_tier(footprint);

    if (tier == TIER_L1_COMPLIANT)
    {
        emit_code(compiler, "/* L1-COMPLIANT: Using direct lookup tables */\n");
        generate_l1_kernel(compiler, schema);
    }
    else if (tier == TIER_L2_COMPLIANT)
    {
        emit_code(compiler, "/* L2-COMPLIANT: Using bit-vector operations */\n");
        generate_l2_kernel(compiler, schema);
    }
    else if (tier == TIER_L3_COMPLIANT)
    {
        emit_code(compiler, "/* L3-COMPLIANT: Using compressed structures */\n");
        generate_l3_kernel(compiler, schema);
    }
    else
    {
        emit_code(compiler, "/* WARNING: Non-compliant - using sharded approach */\n");
        if (compiler->config.enable_sharding)
        {
            generate_sharded_kernel(compiler, schema);
        }
    }
}

// Generate L1-optimized kernel (everything in hot cache)
static void generate_l1_kernel(SevenTickCompiler *compiler,
                               const SchemaDefinition *schema)
{
    // Static data structures that fit in L1
    emit_code(compiler, "/* Static L1-resident data structures */\n");
    emit_code(compiler, "static const uint32_t ps_to_o_index[%zu] = {\n",
              schema->expected_predicates * schema->expected_subjects);
    emit_code(compiler, "    /* Initialized at compile time */\n");
    emit_code(compiler, "};\n\n");

    // Ultra-fast lookup function
    emit_code(compiler, "static inline int ask_pattern_l1(uint32_t s, uint32_t p, uint32_t o) {\n");
    emit_code(compiler, "    /* 7-tick guarantee when in L1 */\n");
    emit_code(compiler, "    uint32_t stored_o = ps_to_o_index[p * %zu + s];\n",
              schema->expected_subjects);
    emit_code(compiler, "    return (stored_o == o);\n");
    emit_code(compiler, "}\n\n");
}

// Generate L2-optimized kernel (bit-vectors in cache)
static void generate_l2_kernel(SevenTickCompiler *compiler,
                               const SchemaDefinition *schema)
{
    size_t stride = (schema->expected_subjects + 63) / 64;

    emit_code(compiler, "/* L2-resident bit-vector structures */\n");
    emit_code(compiler, "static uint64_t predicate_vectors[%zu][%zu];\n",
              schema->expected_predicates, stride);
    emit_code(compiler, "static uint64_t object_vectors[%zu][%zu];\n\n",
              schema->expected_objects, stride);

    // Bit-vector join function
    emit_code(compiler, "static inline void join_patterns_l2(\n");
    emit_code(compiler, "    uint32_t p1, uint32_t o1,\n");
    emit_code(compiler, "    uint32_t p2, uint32_t o2,\n");
    emit_code(compiler, "    uint64_t* result) {\n");
    emit_code(compiler, "    /* Vectorized AND operation */\n");
    emit_code(compiler, "    for (size_t i = 0; i < %zu; i++) {\n", stride);
    emit_code(compiler, "        uint64_t v1 = predicate_vectors[p1][i] & object_vectors[o1][i];\n");
    emit_code(compiler, "        uint64_t v2 = predicate_vectors[p2][i] & object_vectors[o2][i];\n");
    emit_code(compiler, "        result[i] = v1 & v2;\n");
    emit_code(compiler, "    }\n");
    emit_code(compiler, "}\n\n");
}

// Generate L3-optimized kernel (compressed structures)
static void generate_l3_kernel(SevenTickCompiler *compiler,
                               const SchemaDefinition *schema)
{
    emit_code(compiler, "/* L3-resident compressed structures */\n");
    emit_code(compiler, "/* Using sparse representations and compression */\n\n");

    // Compressed Sparse Row (CSR) for predicate-subject matrix
    emit_code(compiler, "/* Compressed Sparse Row (CSR) representation */\n");
    emit_code(compiler, "static uint32_t ps_row_ptr[%zu];\n", schema->expected_predicates + 1);
    emit_code(compiler, "static uint32_t ps_col_ind[%zu];\n", schema->expected_predicates * 100); // Estimate
    emit_code(compiler, "static uint32_t ps_values[%zu];\n\n", schema->expected_predicates * 100);

    // Run-length encoded bit vectors for sparse data
    emit_code(compiler, "/* Run-length encoded bit vectors */\n");
    emit_code(compiler, "typedef struct {\n");
    emit_code(compiler, "    uint32_t run_length;\n");
    emit_code(compiler, "    uint8_t value;\n");
    emit_code(compiler, "} RLEEntry;\n\n");

    emit_code(compiler, "static RLEEntry object_rle[%zu];\n", schema->expected_objects * 2); // Estimate
    emit_code(compiler, "static uint32_t rle_count = 0;\n\n");

    // Dictionary encoding for repeated values
    emit_code(compiler, "/* Dictionary encoding for repeated values */\n");
    emit_code(compiler, "static uint32_t dict_values[%zu];\n", 1000); // Common values
    emit_code(compiler, "static uint32_t dict_count = 0;\n");
    emit_code(compiler, "static uint32_t dict_map[%zu];\n\n", schema->expected_objects);

    // Compressed lookup function
    emit_code(compiler, "static inline int ask_pattern_l3_compressed(uint32_t s, uint32_t p, uint32_t o) {\n");
    emit_code(compiler, "    /* CSR lookup for predicate-subject */\n");
    emit_code(compiler, "    uint32_t start = ps_row_ptr[p];\n");
    emit_code(compiler, "    uint32_t end = ps_row_ptr[p + 1];\n");
    emit_code(compiler, "    \n");
    emit_code(compiler, "    /* Binary search in sorted column indices */\n");
    emit_code(compiler, "    while (start < end) {\n");
    emit_code(compiler, "        uint32_t mid = (start + end) / 2;\n");
    emit_code(compiler, "        if (ps_col_ind[mid] == s) {\n");
    emit_code(compiler, "            /* Found subject, check object */\n");
    emit_code(compiler, "            uint32_t stored_o = ps_values[mid];\n");
    emit_code(compiler, "            return (stored_o == o);\n");
    emit_code(compiler, "        } else if (ps_col_ind[mid] < s) {\n");
    emit_code(compiler, "            start = mid + 1;\n");
    emit_code(compiler, "        } else {\n");
    emit_code(compiler, "            end = mid;\n");
    emit_code(compiler, "        }\n");
    emit_code(compiler, "    }\n");
    emit_code(compiler, "    return 0;\n");
    emit_code(compiler, "}\n\n");

    // RLE bit vector operations
    emit_code(compiler, "static inline int rle_bit_test(uint32_t index) {\n");
    emit_code(compiler, "    uint32_t current_pos = 0;\n");
    emit_code(compiler, "    for (uint32_t i = 0; i < rle_count; i++) {\n");
    emit_code(compiler, "        if (index < current_pos + object_rle[i].run_length) {\n");
    emit_code(compiler, "            return object_rle[i].value;\n");
    emit_code(compiler, "        }\n");
    emit_code(compiler, "        current_pos += object_rle[i].run_length;\n");
    emit_code(compiler, "    }\n");
    emit_code(compiler, "    return 0;\n");
    emit_code(compiler, "}\n\n");

    // Dictionary lookup
    emit_code(compiler, "static inline uint32_t dict_lookup(uint32_t encoded_value) {\n");
    emit_code(compiler, "    if (encoded_value < dict_count) {\n");
    emit_code(compiler, "        return dict_values[encoded_value];\n");
    emit_code(compiler, "    }\n");
    emit_code(compiler, "    return encoded_value; // Fallback to direct value\n");
    emit_code(compiler, "}\n\n");
}

// Generate sharded kernel for non-compliant data
static void generate_sharded_kernel(SevenTickCompiler *compiler,
                                    const SchemaDefinition *schema)
{
    emit_code(compiler, "/* SHARDED KERNEL - Data split across multiple L2/L3 kernels */\n");
    emit_code(compiler, "#define NUM_SHARDS 4\n\n");

    size_t subjects_per_shard = schema->expected_subjects / 4;
    emit_code(compiler, "/* Each shard handles %zu subjects */\n", subjects_per_shard);

    emit_code(compiler, "static inline int get_shard(uint32_t subject) {\n");
    emit_code(compiler, "    return subject %% NUM_SHARDS;\n");
    emit_code(compiler, "}\n\n");
}

// Compile the complete kernel
CompiledKernel *compiler_compile(SevenTickCompiler *compiler,
                                 const SchemaDefinition *schema)
{
    CompiledKernel *kernel = calloc(1, sizeof(CompiledKernel));

    // Analyze footprint
    compiler_analyze_schema(compiler, schema, &kernel->footprint);

    // Generate code
    generate_kernel_code(compiler, schema, &kernel->footprint);

    // Generate certificate
    char kernel_name[256];
    snprintf(kernel_name, sizeof(kernel_name), "kernel_%zu_%zu_%zu",
             schema->num_classes, schema->num_shapes, schema->expected_subjects);
    generate_compliance_certificate(kernel_name, &kernel->footprint,
                                    &kernel->certificate);

    // Save generated code
    kernel->code = strdup(compiler->code_buffer);
    kernel->code_size = compiler->code_size;

    return kernel;
}

// Write kernel to file
void compiler_write_kernel(const CompiledKernel *kernel, const char *filename)
{
    FILE *f = fopen(filename, "w");
    if (!f)
        return;

    // Write compliance certificate as comment
    fprintf(f, "/*\n");
    fprintf(f, " * 7T COMPLIANCE CERTIFICATE\n");
    fprintf(f, " * Kernel: %s\n", kernel->certificate.kernel_name);
    fprintf(f, " * Tier: %s\n", TIER_SPECS[kernel->certificate.certified_tier].name);
    fprintf(f, " * Footprint: %zu KB\n", kernel->footprint.total_footprint_kb);
    fprintf(f, " * Guaranteed Latency: < %.0f ns\n", kernel->certificate.guaranteed_latency_ns);
    fprintf(f, " * Guaranteed Throughput: > %llu M ops/sec\n",
            kernel->certificate.guaranteed_throughput_ops_sec / 1000000);
    fprintf(f, " */\n\n");

    // Write generated code
    fwrite(kernel->code, 1, kernel->code_size, f);

    fclose(f);
}

// Suggest optimizations to meet tier requirements
void compiler_suggest_optimizations(SevenTickCompiler *compiler,
                                    const SchemaDefinition *schema,
                                    ComplianceTier target_tier)
{
    KernelFootprint footprint;
    compiler_analyze_schema(compiler, schema, &footprint);

    ComplianceTier current_tier = calculate_compliance_tier(&footprint);

    printf("\n=== Optimization Suggestions ===\n");
    printf("Current footprint: %zu KB (Tier: %s)\n",
           footprint.total_footprint_kb,
           TIER_SPECS[current_tier].name);
    printf("Target tier: %s (max %zu KB)\n",
           TIER_SPECS[target_tier].name,
           TIER_SPECS[target_tier].max_footprint_kb);

    if (current_tier > target_tier)
    {
        size_t reduction_needed = footprint.total_footprint_kb -
                                  TIER_SPECS[target_tier].max_footprint_kb;
        printf("\nNeed to reduce footprint by %zu KB\n", reduction_needed);

        printf("\nSuggestions:\n");

        // Analyze biggest contributors
        if (footprint.sparql_index_kb > footprint.total_footprint_kb * 0.5)
        {
            printf("- Remove PS->O index, use bit-vector joins instead\n");
            printf("  Savings: %zu KB\n", footprint.sparql_index_kb);
        }

        if (footprint.sparql_object_vectors_kb > 100)
        {
            size_t reduced_objects = schema->expected_objects / 4;
            printf("- Reduce object count from %zu to %zu\n",
                   schema->expected_objects, reduced_objects);
            printf("  Savings: ~%zu KB\n",
                   footprint.sparql_object_vectors_kb * 3 / 4);
        }

        if (schema->num_classes > 1000)
        {
            printf("- Simplify ontology hierarchy (currently %zu classes)\n",
                   schema->num_classes);
            printf("- Consider splitting into multiple smaller ontologies\n");
        }

        printf("- Enable sharding to split data across multiple kernels\n");
        printf("- Use compression for sparse data\n");
    }
    else
    {
        printf("\n✅ Schema already fits in target tier!\n");
    }
}

// Cleanup
void compiler_destroy(SevenTickCompiler *compiler)
{
    if (compiler)
    {
        free(compiler->code_buffer);
        free(compiler);
    }
}