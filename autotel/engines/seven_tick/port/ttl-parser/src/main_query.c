// TTL Parser - Main Program with Query Engine
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "parser.h"
#include "ast.h"
#include "visitor.h"
#include "query.h"

// Program options
typedef struct {
    const char* input_file;
    const char* output_file;
    const char* query_string;
    bool json_output;
    bool validate_only;
    bool show_stats;
    bool quiet;
    bool query_mode;
} Options;

// Print usage information
static void print_usage(const char* program_name) {
    fprintf(stderr, "TTL Parser with Query Engine v1.0\n");
    fprintf(stderr, "Usage: %s [OPTIONS] input.ttl\n", program_name);
    fprintf(stderr, "\nQuery Options:\n");
    fprintf(stderr, "  -q, --query PATTERN    Execute query pattern (e.g., \"?s rdf:type foaf:Person\")\n");
    fprintf(stderr, "  -j, --json             Output results as JSON\n");
    fprintf(stderr, "\nGeneral Options:\n");
    fprintf(stderr, "  -o, --output FILE      Write output to FILE\n");
    fprintf(stderr, "  -v, --validate         Validate only, don't output AST\n");
    fprintf(stderr, "  -S, --stats            Show parsing and query statistics\n");
    fprintf(stderr, "  --quiet                Suppress non-error output\n");
    fprintf(stderr, "  -h, --help             Show this help message\n");
    fprintf(stderr, "\nQuery Examples:\n");
    fprintf(stderr, "  %s --query \"?s rdf:type foaf:Person\" input.ttl\n", program_name);
    fprintf(stderr, "  %s --query \"?s foaf:name ?name\" input.ttl\n", program_name);
    fprintf(stderr, "  %s --query \"<http://example.org/person1> ?p ?o\" input.ttl\n", program_name);
    fprintf(stderr, "  %s --query \"?s a ?type\" --json input.ttl\n", program_name);
}

// Parse command line options
static bool parse_options(int argc, char* argv[], Options* opts) {
    static struct option long_options[] = {
        {"output",    required_argument, 0, 'o'},
        {"query",     required_argument, 0, 'q'},
        {"json",      no_argument,       0, 'j'},
        {"validate",  no_argument,       0, 'v'},
        {"stats",     no_argument,       0, 'S'},
        {"quiet",     no_argument,       0, 1000},
        {"help",      no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };
    
    // Initialize defaults
    memset(opts, 0, sizeof(Options));
    
    int c;
    while ((c = getopt_long(argc, argv, "o:q:jvSh", long_options, NULL)) != -1) {
        switch (c) {
            case 'o':
                opts->output_file = optarg;
                break;
            case 'q':
                opts->query_string = optarg;
                opts->query_mode = true;
                break;
            case 'j':
                opts->json_output = true;
                break;
            case 'v':
                opts->validate_only = true;
                break;
            case 'S':
                opts->show_stats = true;
                break;
            case 1000: // --quiet
                opts->quiet = true;
                break;
            case 'h':
                print_usage(argv[0]);
                exit(0);
            default:
                return false;
        }
    }
    
    if (optind >= argc) {
        fprintf(stderr, "Error: No input file specified\n");
        return false;
    }
    
    opts->input_file = argv[optind];
    return true;
}

// Execute query and print results
static void execute_query(ttl_query_engine_t *engine, const char *query_string, 
                         FILE *output, bool json_format) {
    if (!query_string) {
        fprintf(stderr, "No query specified\n");
        return;
    }
    
    printf("Executing query: %s\n", query_string);
    
    ttl_query_result_t *result = ttl_query_execute_simple(engine, query_string);
    if (!result) {
        fprintf(stderr, "Query execution failed\n");
        return;
    }
    
    size_t count = ttl_query_result_count(result);
    if (count == 0) {
        fprintf(output, "No results found.\n");
    } else {
        if (json_format) {
            ttl_query_result_print_json(result, output);
        } else {
            ttl_query_result_print(result, output);
        }
    }
    
    ttl_query_result_destroy(result);
}

// Main program
int main(int argc, char* argv[]) {
    Options opts;
    int exit_code = 0;
    
    // Parse command line options
    if (!parse_options(argc, argv, &opts)) {
        print_usage(argv[0]);
        return 1;
    }
    
    if (!opts.quiet) {
        printf("TTL Parser with Query Engine\n");
        printf("Parsing: %s\n", opts.input_file);
    }
    
    // Read file content into buffer (same approach as main parser)
    FILE *input_file = fopen(opts.input_file, "r");
    if (!input_file) {
        fprintf(stderr, "Error: Cannot open input file: %s\n", opts.input_file);
        return 1;
    }
    
    if (!opts.quiet) {
        printf("Parsing TTL file with real parser...\n");
    }
    
    // Read file content
    fseek(input_file, 0, SEEK_END);
    long length = ftell(input_file);
    fseek(input_file, 0, SEEK_SET);
    
    char* content = malloc(length + 1);
    if (!content) {
        fprintf(stderr, "Error: Cannot allocate memory for file content\n");
        fclose(input_file);
        return 1;
    }
    
    size_t read_size = fread(content, 1, length, input_file);
    content[read_size] = '\0';
    fclose(input_file);
    
    // Create parser with default options
    ParserOptions parser_opts = ttl_parser_default_options();
    parser_opts.error_recovery = true;  // Enable error recovery for better experience
    
    Parser *parser = ttl_parser_create(content, read_size, &parser_opts);
    
    if (!parser) {
        fprintf(stderr, "Error: Failed to create parser\n");
        free(content);
        return 1;
    }
    
    // Parse the document and get real AST
    ttl_ast_node_t *document = ttl_parser_parse(parser);
    if (!document) {
        fprintf(stderr, "Error: Failed to parse TTL document\n");
        
        // Print parser errors for debugging
        if (ttl_parser_has_errors(parser)) {
            const ParseError *error = ttl_parser_get_errors(parser);
            while (error) {
                fprintf(stderr, "Parse error at line %d, column %d: %s\n",
                       error->line, error->column, error->message);
                error = error->next;
            }
        }
        
        ttl_parser_destroy(parser);
        free(content);
        return 1;
    }
    
    // DEBUG: Check document immediately after parsing
    printf("DEBUG: Document parsed, type = %d\n", document->type);
    if (document->type == TTL_AST_DOCUMENT) {
        printf("DEBUG: Document statement_count = %zu\n", document->data.document.statement_count);
    } else {
        printf("DEBUG: Document is not TTL_AST_DOCUMENT type!\n");
    }
    
    // Create an AST context for the query engine
    // The parser manages its own context, but for querying we create our own
    ttl_ast_context_t *context = ttl_ast_context_create(true);
    
    // Get parsing statistics
    ParserStats stats;
    ttl_parser_get_stats(parser, &stats);
    
    if (!opts.quiet) {
        printf("Successfully parsed TTL file!\n");
        printf("  Statements parsed: %zu\n", stats.statements_parsed);
        printf("  Triples parsed: %zu\n", stats.triples_parsed);
        if (stats.errors_recovered > 0) {
            printf("  Errors recovered: %zu\n", stats.errors_recovered);
        }
    }
    
    // DEBUG: Check document before creating query engine
    printf("DEBUG: About to create query engine with document type = %d\n", document->type);
    if (document->type == TTL_AST_DOCUMENT) {
        printf("DEBUG: Document still has statement_count = %zu\n", document->data.document.statement_count);
    }
    
    // Create query engine
    ttl_query_engine_t *query_engine = ttl_query_engine_create(document, context);
    if (!query_engine) {
        fprintf(stderr, "Error: Failed to create query engine\n");
        ttl_parser_destroy(parser);  // Clean up parser first
        ttl_ast_context_destroy(context);
        free(content);
        return 1;
    }
    
    if (!opts.quiet) {
        printf("Query engine initialized\n");
    }
    
    // Open output file if specified
    FILE* output = stdout;
    if (opts.output_file) {
        output = fopen(opts.output_file, "w");
        if (!output) {
            fprintf(stderr, "Error: Cannot open output file: %s\n", opts.output_file);
            exit_code = 1;
            goto cleanup;
        }
    }
    
    if (opts.query_mode) {
        // Execute query
        execute_query(query_engine, opts.query_string, output, opts.json_output);
    } else if (!opts.validate_only) {
        // Print parsed AST
        if (opts.json_output) {
            fprintf(output, "{\n\"message\": \"AST parsing not implemented in this demo\",\n");
            fprintf(output, "\"note\": \"Use --query option to test query functionality\"\n}\n");
        } else {
            fprintf(output, "Parsing successful! Use --query option to test query functionality.\n");
            fprintf(output, "Example: --query \"?s rdf:type foaf:Person\"\n");
        }
    }
    
    // Show statistics if requested
    if (opts.show_stats) {
        size_t queries_executed, patterns_matched, total_results;
        ttl_query_engine_get_stats(query_engine, &queries_executed, &patterns_matched, &total_results);
        
        fprintf(stderr, "\\nQuery Engine Statistics:\\n");
        fprintf(stderr, "  Queries executed: %zu\\n", queries_executed);
        fprintf(stderr, "  Patterns matched: %zu\\n", patterns_matched);
        fprintf(stderr, "  Total results: %zu\\n", total_results);
    }
    
cleanup:
    // Clean up
    if (output != stdout && output) {
        fclose(output);
    }
    ttl_query_engine_destroy(query_engine);
    ttl_parser_destroy(parser);  // Destroy parser after query engine
    ttl_ast_context_destroy(context);
    free(content);  // Free the file content buffer
    
    return exit_code;
}