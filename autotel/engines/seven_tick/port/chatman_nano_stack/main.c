#include "topology_lifter.h"
#include "registry.h"
#include "conductor_manifest.h"
#include "fiber_engine.h"
#include "meta_probe.h"
#include "contract_resolver.h" // Include for contract_resolver_init
#include <stdio.h>
#include <stdlib.h>

// Global meta-probe collector instance
meta_probe_collector_t g_meta_probe_collector;

int main() {
    printf("\n--- BitActor Causal Stack Simulation ---\n");

    // 1. Initialize L8 Telemetry
    l8_telemetry_init();

    // 2. Initialize L6 Registry
    registry_init();

    // 3. Initialize L4 Meta-Probe Collector
    meta_probe_init(&g_meta_probe_collector, "BitActorSimulation");

    // 4. Initialize L3 Contract Resolver
    contract_resolver_init();

    // 4. Register a dummy actor in L6 Registry
    // In a real system, memory_location would point to the actor's actual state in memory.
    void* dummy_actor_state = (void*)0xDEADBEEF; // Placeholder address
    registry_register_actor("MyActor", 101, dummy_actor_state);

    // 5. Prepare dummy L2 Mailbox and L5 Manifest Entry
    actor_mailbox_t my_mailbox = {
        .message = (void*)0xCAFEF00D, // Dummy message
        .message_id = 1,
        .has_message = 1
    };

    // Get a manifest entry (using the first one from conductor_manifest.c)
    const conductor_manifest_entry_t* my_manifest_entry = &conductor_manifest[0];

    printf("\n--- Executing Fiber ---\n");
    // 6. Call L2 Fiber Engine to execute the actor's fiber
    int result = fiber_engine_execute_fiber(
        101, // actor_id
        dummy_actor_state,
        &my_mailbox,
        my_manifest_entry,
        &g_meta_probe_collector // Pass the collector
    );

    printf("\n--- Simulation Results ---\n");
    if (result == 0) {
        printf("Fiber execution completed successfully and within budget.\n");
    } else {
        printf("Fiber execution failed or exceeded budget.\n");
    }

    // Get L4 metrics
    struct meta_probe_metrics metrics;
    meta_probe_get_metrics(&g_meta_probe_collector, &metrics);
    printf("Total spans created: %lu\n", metrics.total_spans);
    printf("Successful spans (within causal bounds): %lu\n", metrics.successful_spans);
    printf("Genesis resets triggered: %lu\n", metrics.genesis_resets);

    // 7. Shutdown L8 Telemetry
    l8_telemetry_shutdown();

    // 8. Shutdown L6 Registry
    registry_shutdown();

    // 9. Cleanup L4 Meta-Probe Collector
    meta_probe_cleanup(&g_meta_probe_collector);

    printf("--- Simulation End ---\n\n");

    return 0;
}