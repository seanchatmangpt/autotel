#include "cns/cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

// ANSI color codes
#define COLOR_RED     "\033[0;31m"
#define COLOR_GREEN   "\033[0;32m"
#define COLOR_YELLOW  "\033[0;33m"
#define COLOR_BLUE    "\033[0;34m"
#define COLOR_RESET   "\033[0m"

// Global state (minimal for performance)
static struct {
    const char* program_name;
    CNSDomain* domains;
    size_t domain_count;
    size_t domain_capacity;
    bool initialized;
} g_cli = {0};

// Initialize CLI framework
int cns_cli_init(const char* program_name) {
    if (g_cli.initialized) {
        return CNS_ERROR;
    }
    
    g_cli.program_name = program_name;
    g_cli.domain_capacity = 16;
    g_cli.domains = calloc(g_cli.domain_capacity, sizeof(CNSDomain));
    if (!g_cli.domains) {
        return CNS_ERROR_MEMORY;
    }
    
    g_cli.initialized = true;
    return CNS_OK;
}

// Register a domain
int cns_cli_register_domain(const CNSDomain* domain) {
    if (!g_cli.initialized || !domain) {
        return CNS_ERROR;
    }
    
    // Check for duplicates
    for (size_t i = 0; i < g_cli.domain_count; i++) {
        if (strcmp(g_cli.domains[i].name, domain->name) == 0) {
            return CNS_ERROR;
        }
    }
    
    // Expand if needed
    if (g_cli.domain_count >= g_cli.domain_capacity) {
        size_t new_capacity = g_cli.domain_capacity * 2;
        CNSDomain* new_domains = realloc(g_cli.domains, new_capacity * sizeof(CNSDomain));
        if (!new_domains) {
            return CNS_ERROR_MEMORY;
        }
        g_cli.domains = new_domains;
        g_cli.domain_capacity = new_capacity;
    }
    
    // Copy domain
    g_cli.domains[g_cli.domain_count] = *domain;
    g_cli.domain_count++;
    
    return CNS_OK;
}

// Find domain by name
static CNSDomain* find_domain(const char* name) {
    for (size_t i = 0; i < g_cli.domain_count; i++) {
        if (strcmp(g_cli.domains[i].name, name) == 0) {
            return &g_cli.domains[i];
        }
    }
    return NULL;
}

// Find command in domain
static CNSCommand* find_command(CNSDomain* domain, const char* name) {
    for (size_t i = 0; i < domain->command_count; i++) {
        if (strcmp(domain->commands[i].name, name) == 0) {
            return &domain->commands[i];
        }
    }
    return NULL;
}

// Print general help
void cns_cli_print_help(void) {
    printf("%sCHATMAN NANO-STACK (CNS)%s - Performance-First CLI\n\n", COLOR_BLUE, COLOR_RESET);
    printf("Usage: %s <domain> <command> [options] [arguments]\n\n", g_cli.program_name);
    printf("Domains:\n");
    
    for (size_t i = 0; i < g_cli.domain_count; i++) {
        printf("  %-12s %s\n", g_cli.domains[i].name, g_cli.domains[i].description);
    }
    
    printf("\nExamples:\n");
    printf("  %s build init          Initialize build system\n", g_cli.program_name);
    printf("  %s bench all           Run all benchmarks\n", g_cli.program_name);
    printf("  %s parse file.ttl      Parse ontology file\n", g_cli.program_name);
    printf("\nUse '%s <domain> --help' for domain-specific help\n", g_cli.program_name);
}

// Print domain help
void cns_cli_print_domain_help(const char* domain_name) {
    CNSDomain* domain = find_domain(domain_name);
    if (!domain) {
        cns_cli_error("Unknown domain: %s", domain_name);
        return;
    }
    
    printf("%sDomain: %s%s - %s\n\n", COLOR_BLUE, domain->name, COLOR_RESET, domain->description);
    printf("Commands:\n");
    
    for (size_t i = 0; i < domain->command_count; i++) {
        printf("  %-12s %s\n", domain->commands[i].name, domain->commands[i].description);
    }
    
    printf("\nUse '%s %s <command> --help' for command-specific help\n", 
           g_cli.program_name, domain_name);
}

// Print command help
void cns_cli_print_command_help(const char* domain_name, const char* command_name) {
    CNSDomain* domain = find_domain(domain_name);
    if (!domain) {
        cns_cli_error("Unknown domain: %s", domain_name);
        return;
    }
    
    CNSCommand* command = find_command(domain, command_name);
    if (!command) {
        cns_cli_error("Unknown command: %s %s", domain_name, command_name);
        return;
    }
    
    printf("%sCommand: %s %s%s - %s\n\n", 
           COLOR_BLUE, domain_name, command_name, COLOR_RESET, command->description);
    
    // Usage
    printf("Usage: %s %s %s", g_cli.program_name, domain_name, command_name);
    if (command->option_count > 0) printf(" [options]");
    for (size_t i = 0; i < command->argument_count; i++) {
        if (command->arguments[i].required) {
            printf(" <%s>", command->arguments[i].name);
        } else {
            printf(" [%s]", command->arguments[i].name);
        }
    }
    printf("\n");
    
    // Arguments
    if (command->argument_count > 0) {
        printf("\nArguments:\n");
        for (size_t i = 0; i < command->argument_count; i++) {
            printf("  %-12s %s%s\n", 
                   command->arguments[i].name,
                   command->arguments[i].description,
                   command->arguments[i].required ? " (required)" : "");
        }
    }
    
    // Options
    if (command->option_count > 0) {
        printf("\nOptions:\n");
        for (size_t i = 0; i < command->option_count; i++) {
            if (command->options[i].short_name) {
                printf("  -%c, --%-10s %s%s\n",
                       command->options[i].short_name,
                       command->options[i].name,
                       command->options[i].description,
                       command->options[i].required ? " (required)" : "");
            } else {
                printf("      --%-10s %s%s\n",
                       command->options[i].name,
                       command->options[i].description,
                       command->options[i].required ? " (required)" : "");
            }
        }
    }
}

// Run the CLI
int cns_cli_run(int argc, char** argv) {
    if (!g_cli.initialized) {
        return CNS_ERROR;
    }
    
    // Need at least program name
    if (argc < 1) {
        return CNS_ERROR_ARGS;
    }
    
    // No arguments - show help
    if (argc == 1) {
        cns_cli_print_help();
        return CNS_OK;
    }
    
    // Check for global flags
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        cns_cli_print_help();
        return CNS_OK;
    }
    
    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
        cns_cli_print_version();
        return CNS_OK;
    }
    
    // Find domain
    CNSDomain* domain = find_domain(argv[1]);
    if (!domain) {
        cns_cli_error("Unknown domain: %s", argv[1]);
        cns_cli_print_help();
        return CNS_ERROR_NOT_FOUND;
    }
    
    // Domain help
    if (argc == 2 || (argc == 3 && (strcmp(argv[2], "--help") == 0 || strcmp(argv[2], "-h") == 0))) {
        cns_cli_print_domain_help(argv[1]);
        return CNS_OK;
    }
    
    // Find command
    CNSCommand* command = find_command(domain, argv[2]);
    if (!command) {
        cns_cli_error("Unknown command: %s %s", argv[1], argv[2]);
        cns_cli_print_domain_help(argv[1]);
        return CNS_ERROR_NOT_FOUND;
    }
    
    // Command help
    if (argc >= 4 && (strcmp(argv[3], "--help") == 0 || strcmp(argv[3], "-h") == 0)) {
        cns_cli_print_command_help(argv[1], argv[2]);
        return CNS_OK;
    }
    
    // Create context
    CNSContext ctx = {
        .program_name = g_cli.program_name,
        .domain = argv[1],
        .command = argv[2],
        .start_cycles = cns_get_cycles()
    };
    
    // Execute command
    int result = command->handler(&ctx, argc - 3, argv + 3);
    
    // Check 7-tick constraint
    uint64_t elapsed = cns_get_cycles() - ctx.start_cycles;
    if (elapsed > 7) {
        cns_cli_error("PERFORMANCE VIOLATION: Command exceeded 7-tick limit (%llu cycles)", elapsed);
        return CNS_ERROR_CYCLES;
    }
    
    return result;
}

// Cleanup
void cns_cli_cleanup(void) {
    if (g_cli.domains) {
        free(g_cli.domains);
    }
    memset(&g_cli, 0, sizeof(g_cli));
}

// Print version
void cns_cli_print_version(void) {
    printf("CNS (CHATMAN NANO-STACK) version 1.0.0\n");
    printf("7-tick performance-guaranteed CLI\n");
}

// Error printing
void cns_cli_error(const char* format, ...) {
    fprintf(stderr, "%s✗ ERROR:%s ", COLOR_RED, COLOR_RESET);
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
}

// Warning printing
void cns_cli_warning(const char* format, ...) {
    fprintf(stderr, "%s⚠ WARNING:%s ", COLOR_YELLOW, COLOR_RESET);
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
}

// Info printing
void cns_cli_info(const char* format, ...) {
    printf("%sℹ INFO:%s ", COLOR_BLUE, COLOR_RESET);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

// Success printing
void cns_cli_success(const char* format, ...) {
    printf("%s✓ SUCCESS:%s ", COLOR_GREEN, COLOR_RESET);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

// Get CPU cycles (platform-specific)
uint64_t cns_get_cycles(void) {
#if defined(__x86_64__) || defined(__i386__)
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
#elif defined(__aarch64__)
    uint64_t cycles;
    __asm__ __volatile__ ("mrs %0, cntvct_el0" : "=r"(cycles));
    return cycles;
#else
    // Fallback - not cycle-accurate
    return 0;
#endif
}

// Assert cycles are within limit
void cns_assert_cycles(uint64_t start, uint64_t limit) {
    uint64_t elapsed = cns_get_cycles() - start;
    if (elapsed > limit) {
        cns_cli_error("CYCLE VIOLATION: %llu > %llu", elapsed, limit);
        abort();
    }
}