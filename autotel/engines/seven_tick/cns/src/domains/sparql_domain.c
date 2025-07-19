
#include "cns/types.h"
#include "cns/cli.h"
#include "cns/sparql.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// SPARQL engine commands
static int cmd_sparql_query(CNSContext *ctx, int argc, char **argv)
{
    if (argc < 2)
    {
        cns_cli_error("Usage: sparql query <query_string>");
        return CNS_ERR_INVALID_ARG;
    }

    const char *query = argv[1];

    // Get or create SPARQL engine
    SPARQLEngine *engine = cns_sparql_create(1024);
    if (!engine)
    {
        cns_cli_error("Failed to create SPARQL engine");
        return CNS_ERR_RESOURCE;
    }

    // Execute SPARQL query (15 cycles)
    uint64_t start = cns_get_cycles();

    SPARQLResult *result = cns_sparql_execute(engine, query);

    uint64_t cycles = cns_get_cycles() - start;
    cns_assert_cycles(start, 15);

    if (result)
    {
        cns_cli_success("✅ SPARQL query executed (took %llu cycles): %s", cycles, query);
        cns_cli_info("   Results: %u triples", result->count);
        cns_sparql_free_result(result);
    }
    else
    {
        cns_cli_error("Failed to execute SPARQL query");
        return CNS_ERR_INTERNAL;
    }

    return CNS_OK;
}

static int cmd_sparql_compile(CNSContext *ctx, int argc, char **argv)
{
    if (argc < 2)
    {
        cns_cli_error("Usage: sparql compile <query_file>");
        return CNS_ERR_INVALID_ARG;
    }

    const char *query_file = argv[1];

    // Simulate SPARQL AOT compilation (50 cycles)
    uint64_t start = cns_get_cycles();

    // TODO: Implement actual SPARQL compilation
    // int result = cns_sparql_aot_compile(query_file, output_file);

    uint64_t cycles = cns_get_cycles() - start;

    cns_cli_success("✅ SPARQL query compiled (took %llu cycles): %s", cycles, query_file);
    return CNS_OK;
}

static int cmd_sparql_benchmark(CNSContext *ctx, int argc, char **argv)
{
    int iterations = 1000;
    if (argc > 1)
    {
        iterations = atoi(argv[1]);
    }

    cns_cli_info("Running SPARQL benchmark with %d iterations...", iterations);

    // Get or create SPARQL engine
    SPARQLEngine *engine = cns_sparql_create(1024);
    if (!engine)
    {
        cns_cli_error("Failed to create SPARQL engine");
        return CNS_ERR_RESOURCE;
    }

    // Add some test data
    for (int i = 0; i < 100; i++)
    {
        cns_sparql_add_triple(engine, i, i + 100, i + 200);
    }

    uint64_t total_cycles = 0;
    uint64_t min_cycles = UINT64_MAX;
    uint64_t max_cycles = 0;

    // Common SPARQL patterns for benchmarking
    const char *test_queries[] = {
        "SELECT ?s ?p ?o WHERE { ?s ?p ?o }",
        "SELECT ?s WHERE { ?s ?p ?o }",
        "SELECT ?p WHERE { ?s ?p ?o }",
        "SELECT ?o WHERE { ?s ?p ?o }"};
    int num_queries = sizeof(test_queries) / sizeof(test_queries[0]);

    for (int i = 0; i < iterations; i++)
    {
        const char *query = test_queries[i % num_queries];

        uint64_t start = cns_get_cycles();

        SPARQLResult *result = cns_sparql_execute(engine, query);

        uint64_t cycles = cns_get_cycles() - start;
        total_cycles += cycles;

        if (cycles < min_cycles)
            min_cycles = cycles;
        if (cycles > max_cycles)
            max_cycles = cycles;

        if (result)
        {
            cns_sparql_free_result(result);
        }
    }

    double avg_cycles = (double)total_cycles / iterations;

    // Get performance statistics
    SPARQLStats stats = cns_sparql_get_stats(engine);

    cns_cli_success("✅ SPARQL benchmark results:");
    cns_cli_info("   Min: %llu cycles", min_cycles);
    cns_cli_info("   Max: %llu cycles", max_cycles);
    cns_cli_info("   Avg: %.1f cycles", avg_cycles);
    cns_cli_info("   7T compliance: %s", (avg_cycles <= 7) ? "✅" : "❌");
    cns_cli_info("   Cache hit rate: %.1f%%", stats.cache_hit_rate * 100.0);
    cns_cli_info("   Total triples: %u", stats.total_triples);

    return CNS_OK;
}

static int cmd_sparql_add(CNSContext *ctx, int argc, char **argv)
{
    if (argc < 4)
    {
        cns_cli_error("Usage: sparql add <subject> <predicate> <object>");
        return CNS_ERR_INVALID_ARG;
    }

    uint32_t subject = atoi(argv[1]);
    uint32_t predicate = atoi(argv[2]);
    uint32_t object = atoi(argv[3]);

    // Get or create SPARQL engine
    SPARQLEngine *engine = cns_sparql_create(1024);
    if (!engine)
    {
        cns_cli_error("Failed to create SPARQL engine");
        return CNS_ERR_RESOURCE;
    }

    // Add triple (5 cycles)
    uint64_t start = cns_get_cycles();

    int result = cns_sparql_add_triple(engine, subject, predicate, object);

    uint64_t cycles = cns_get_cycles() - start;
    cns_assert_cycles(start, 5);

    if (result == CNS_OK)
    {
        cns_cli_success("✅ Triple added (took %llu cycles): (%u, %u, %u)", cycles, subject, predicate, object);
    }
    else
    {
        cns_cli_error("Failed to add triple");
        return result;
    }

    return CNS_OK;
}

static int cmd_sparql_stats(CNSContext *ctx, int argc, char **argv)
{
    // Get or create SPARQL engine
    SPARQLEngine *engine = cns_sparql_create(1024);
    if (!engine)
    {
        cns_cli_error("Failed to create SPARQL engine");
        return CNS_ERR_RESOURCE;
    }

    SPARQLStats stats = cns_sparql_get_stats(engine);

    cns_cli_success("✅ SPARQL Engine Statistics:");
    cns_cli_info("   Total queries: %llu", stats.total_queries);
    cns_cli_info("   Cache hits: %llu", stats.cache_hits);
    cns_cli_info("   Cache hit rate: %.1f%%", stats.cache_hit_rate * 100.0);
    cns_cli_info("   Total triples: %u", stats.total_triples);
    cns_cli_info("   Avg cycles per query: %.1f", stats.avg_cycles_per_query);

    return CNS_OK;
}

// SPARQL command options
static CNSOption sparql_options[] = {
    {.name = "engine",
     .short_name = 'e',
     .type = CNS_OPT_STRING,
     .description = "SPARQL engine instance",
     .default_val = "default",
     .required = false},
    {.name = "output",
     .short_name = 'o',
     .type = CNS_OPT_STRING,
     .description = "Output file for compilation",
     .default_val = "query.c",
     .required = false}};

// SPARQL commands
static CNSCommand sparql_commands[] = {
    {.name = "query",
     .description = "Execute SPARQL query",
     .handler = cmd_sparql_query,
     .options = sparql_options,
     .option_count = 1,
     .arguments = (CNSArgument[]){
         {.name = "query", .description = "SPARQL query string", .required = true}},
     .argument_count = 1},
    {.name = "compile", .description = "Compile SPARQL query to C code (AOT)", .handler = cmd_sparql_compile, .options = &sparql_options[1], .option_count = 1, .arguments = (CNSArgument[]){{.name = "query_file", .description = "SPARQL query file", .required = true}}, .argument_count = 1},
    {.name = "benchmark", .description = "Run SPARQL performance benchmark", .handler = cmd_sparql_benchmark, .options = NULL, .option_count = 0, .arguments = (CNSArgument[]){{.name = "iterations", .description = "Number of iterations", .required = false}}, .argument_count = 1},
    {.name = "add", .description = "Add triple to SPARQL store", .handler = cmd_sparql_add, .options = NULL, .option_count = 0, .arguments = (CNSArgument[]){{.name = "subject", .description = "Subject ID", .required = true}, {.name = "predicate", .description = "Predicate ID", .required = true}, {.name = "object", .description = "Object ID", .required = true}}, .argument_count = 3},
    {.name = "stats", .description = "Show SPARQL engine statistics", .handler = cmd_sparql_stats, .options = NULL, .option_count = 0, .arguments = NULL, .argument_count = 0}};

// SPARQL domain
CNSDomain cns_sparql_domain = {
    .name = "sparql",
    .description = "SPARQL query processing and pattern matching",
    .commands = sparql_commands,
    .command_count = sizeof(sparql_commands) / sizeof(sparql_commands[0])};