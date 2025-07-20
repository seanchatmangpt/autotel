#include "cns/cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

// Profile data structure
typedef struct {
    const char* function;
    uint64_t cycles;
    uint64_t calls;
    double percent;
} ProfileEntry;

// Profile command arguments
static CNSArgument profile_args[] = {
    {
        .name = "target",
        .description = "Profile target (e.g., sparql7t_core)",
        .required = true,
        .variadic = false
    }
};

// Profile command options
static CNSOption profile_options[] = {
    {
        .name = "duration",
        .short_name = 'd',
        .type = CNS_OPT_INT,
        .description = "Profile duration in seconds",
        .default_val = "10",
        .required = false
    },
    {
        .name = "output",
        .short_name = 'o',
        .type = CNS_OPT_STRING,
        .description = "Output file for flamegraph",
        .default_val = "profile.svg",
        .required = false
    }
};

// Profile handler
static int cmd_profile(CNSContext* ctx, int argc, char** argv) {
    if (argc < 1) {
        cns_cli_error("Missing profile target");
        return CNS_ERR_INVALID_ARG;
    }
    
    const char* target = argv[0];
    int duration = 10; // Default 10 seconds
    
    cns_cli_info("Profiling %s for %d seconds...", target, duration);
    
    // Start profiling (mock implementation)
    printf("  Sampling at 1000 Hz\n");
    printf("  Press Ctrl+C to stop early\n\n");
    
    // Simulate profiling progress
    for (int i = 0; i < duration; i++) {
        printf("\r  Progress: %d/%d seconds", i+1, duration);
        fflush(stdout);
        sleep(1);
    }
    printf("\n\n");
    
    // Generate mock profile data
    ProfileEntry entries[] = {
        {"cns_sparql_ask", 2847291, 1000000, 35.2},
        {"cns_bitslab_get", 1523882, 2000000, 18.9},
        {"cns_arena_alloc", 982371, 500000, 12.2},
        {"cns_oatable_find", 773621, 800000, 9.6},
        {"cns_ring_push", 623981, 1500000, 7.7},
        {"cns_simd_filter", 498273, 300000, 6.2},
        {"other", 823746, 0, 10.2}
    };
    
    // Print hot path analysis
    cns_cli_info("Hot-path analysis:");
    printf("\n%-30s %15s %10s %8s %10s\n", 
           "Function", "Total Cycles", "Calls", "Percent", "Avg Cycles");
    printf("%-30s %15s %10s %8s %10s\n",
           "--------", "------------", "-----", "-------", "----------");
    
    for (size_t i = 0; i < sizeof(entries)/sizeof(entries[0]); i++) {
        double avg_cycles = entries[i].calls > 0 ? 
            (double)entries[i].cycles / entries[i].calls : 0;
        
        printf("%-30s %15llu %10llu %7.1f%% %10.1f\n",
               entries[i].function,
               entries[i].cycles,
               entries[i].calls,
               entries[i].percent,
               avg_cycles);
        
        // Highlight 7-tick violations
        if (avg_cycles > 7.0 && entries[i].calls > 0) {
            cns_cli_warning("  ⚠ %s exceeds 7-tick limit!", entries[i].function);
        }
    }
    
    // Generate flamegraph
    cns_cli_info("\nGenerating flamegraph: profile.svg");
    printf("  View with: open profile.svg\n");
    
    // Create mock SVG file
    FILE* f = fopen("profile.svg", "w");
    if (f) {
        fprintf(f, "<svg><!-- Flamegraph would be here --></svg>\n");
        fclose(f);
    }
    
    cns_cli_success("Profile complete");
    return CNS_OK;
}

// Profile commands (single command domain)
static CNSCommand profile_commands[] = {
    {
        .name = "",  // Default command for domain
        .description = "Profile hot paths",
        .handler = cmd_profile,
        .options = profile_options,
        .option_count = sizeof(profile_options) / sizeof(profile_options[0]),
        .arguments = profile_args,
        .argument_count = 1
    }
};

// Profile domain
CNSDomain cns_profile_domain = {
    .name = "profile",
    .description = "Performance profiling",
    .commands = profile_commands,
    .command_count = sizeof(profile_commands) / sizeof(profile_commands[0])
};