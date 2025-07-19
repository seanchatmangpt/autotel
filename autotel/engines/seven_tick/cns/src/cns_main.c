/*  ─────────────────────────────────────────────────────────────
    cns_main.c  –  CNS Main Entry Point with OpenTelemetry (v1.0)
    7-tick compliant command dispatch with telemetry
    ───────────────────────────────────────────────────────────── */

#include "../include/cns.h"
#include "../include/cns_parser.h"
#include "../include/cns_commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WITH_OPENTELEMETRY
// Forward declarations for OpenTelemetry (C-compatible interface)
void* g_main_tracer;
#endif

/*═══════════════════════════════════════════════════════════════
  OpenTelemetry Initialization
  ═══════════════════════════════════════════════════════════════*/

void cns_main_init_telemetry(void)
{
#ifdef WITH_OPENTELEMETRY
    // TODO: Initialize OpenTelemetry C bindings when available
    g_main_tracer = NULL; // Placeholder
    printf("INFO: OpenTelemetry main telemetry initialized (stub)\n");
#else
    // No-op when OpenTelemetry is disabled
#endif
}

/*═══════════════════════════════════════════════════════════════
  Enhanced Command Dispatch
  ═══════════════════════════════════════════════════════════════*/

int main(int argc, char** argv)
{
    // Initialize telemetry
    cns_main_init_telemetry();
    
    // Initialize parser telemetry  
    cns_parser_init_telemetry();
    
#ifdef WITH_OPENTELEMETRY
    printf("CNS starting with OpenTelemetry support...\n");
#else
    printf("CNS starting without OpenTelemetry...\n");
#endif

    // Initialize CLI framework
    if (cns_cli_init(argv[0]) != CNS_OK) {
        fprintf(stderr, "Failed to initialize CLI framework\n");
        return 1;
    }

    // Register domains (external declarations)
    extern CNSDomain cns_sparql_domain;

    cns_cli_register_domain(&cns_sparql_domain);

    // Run the CLI
    int result = cns_cli_run(argc, argv);

    // Cleanup
    cns_cli_cleanup();
    
#ifdef WITH_OPENTELEMETRY
    // TODO: Cleanup OpenTelemetry resources
    g_main_tracer = NULL;
    printf("OpenTelemetry cleaned up\n");
#endif

    return result;
}