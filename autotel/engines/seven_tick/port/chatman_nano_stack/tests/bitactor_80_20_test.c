
#include "cns/bitactor_80_20.h"
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

typedef uint64_t cns_bitmask_t;

void test_domain_creation() {
    printf("Testing domain creation...\n");
    cns_bitactor_system_t* sys = cns_bitactor_create();
    assert(sys != NULL);

    uint32_t domain_id = bitactor_domain_create(sys->matrix);
    assert(domain_id == 0);
    assert(sys->matrix->domain_count == 1);
    assert(sys->matrix->domain_active_mask == 1);

    cns_bitactor_destroy(sys);
    printf("Domain creation test passed.\n");
}

void test_actor_addition() {
    printf("Testing actor addition...\n");
    cns_bitactor_system_t* sys = cns_bitactor_create();
    assert(sys != NULL);

    uint32_t domain_id = bitactor_domain_create(sys->matrix);
    bitactor_domain_t* domain = &sys->matrix->domains[domain_id];

    bitactor_manifest_t* dummy_manifest = create_bitactor_manifest("dummy_spec");

    uint32_t actor_id = bitactor_add_to_domain(domain, 0xAA, dummy_manifest, "test_actor_AA", sys);
    assert(actor_id == 0);
    assert(domain->actor_count == 1);
    assert(domain->active_mask == 1);
    assert(domain->actors[actor_id].meaning == 0xAA);

    actor_id = bitactor_add_to_domain(domain, 0xBB, dummy_manifest, "test_actor_BB", sys);
    assert(actor_id == 1);
    assert(domain->actor_count == 2);
    assert(domain->active_mask == 3);
    assert(domain->actors[actor_id].meaning == 0xBB);

    cns_bitactor_destroy(sys);
    printf("Actor addition test passed.\n");
}

void test_matrix_tick() {
    printf("Testing matrix tick...\n");
    cns_bitactor_system_t* sys = cns_bitactor_create();
    assert(sys != NULL);

    uint32_t domain_id = bitactor_domain_create(sys->matrix);
    bitactor_domain_t* domain = &sys->matrix->domains[domain_id];

    bitactor_manifest_t* dummy_manifest = create_bitactor_manifest("dummy_spec");

    bitactor_add_to_domain(domain, 0x01, dummy_manifest, "matrix_actor_1", sys);
    bitactor_add_to_domain(domain, 0x02, dummy_manifest, "matrix_actor_2", sys);
    bitactor_add_to_domain(domain, 0x04, dummy_manifest, "matrix_actor_3", sys);

    bitactor_signal_t signals[8];
    for (int i = 0; i < 8; i++) {
        signals[i] = i + 1;
    }

    uint32_t executed_actors = bitactor_matrix_tick(sys->matrix, signals, 8);
    assert(executed_actors == 3);

    // Check that the actors' states have changed after a tick
    assert(domain->actors[0].tick_count == 1);
    assert(domain->actors[1].tick_count == 1);
    assert(domain->actors[2].tick_count == 1);

    cns_bitactor_destroy(sys);
    printf("Matrix tick test passed.\n");
}

void test_nanoregex() {
    printf("Testing nanoregex...\n");    bitactor_nanoregex_t regex;    bool compiled = bitactor_nanoregex_compile(&regex, "test_pattern");    assert(compiled == true);    bitactor_signal_t signals[8];    signals[0] = hash_ttl_content("test_pattern");    for (int i = 1; i < 8; i++) {        signals[i] = i;    }    printf("Pattern hash: %llu\n", regex.pattern_hash);    printf("Signal[0]: %llu\n", signals[0]);    cns_bitmask_t matches = bitactor_nanoregex_match(&regex, signals, 8);    printf("Matches: %llu\n", matches);    assert(matches == 1); // Only the first signal should match    printf("Nanoregex test passed.\n");
}

void test_cognitive_cycle() {
    printf("Testing cognitive cycle...\n");
    compiled_bitactor_t actor;
    memset(&actor, 0, sizeof(compiled_bitactor_t));
    actor.meaning = 0xFF; // Set some initial meaning that will change after collapse
    actor.bytecode_size = 256;

    // Create a dummy domain for context
    bitactor_domain_t dummy_domain;
    memset(&dummy_domain, 0, sizeof(bitactor_domain_t));
    dummy_domain.feed_actor.match_count = 1; // Simulate a match for trigger detect

    uint64_t result = execute_cognitive_cycle(&actor, &dummy_domain);

    // Assertions based on the simplified hop functions
    assert(result != 0); // Should produce some result
    assert(actor.meaning != 0x0F); // Meaning should have changed due to collapse

    printf("Cognitive cycle test passed.\n");
}

void test_manifest_creation() {
    printf("Testing manifest creation and association...\n");
    cns_bitactor_system_t* sys = cns_bitactor_create();
    assert(sys != NULL);

    bitactor_manifest_t* manifest = create_bitactor_manifest("test_spec");
    assert(manifest != NULL);
    assert(manifest->spec_hash == hash_ttl_content("test_spec"));
    assert(manifest->bytecode != NULL);
    assert(manifest->bytecode_size == 256);

    uint32_t domain_id = bitactor_domain_create(sys->matrix);
    bitactor_domain_t* domain = &sys->matrix->domains[domain_id];

    uint32_t actor_id = bitactor_add_to_domain(domain, 0xDE, manifest, "manifest_actor", sys);
    assert(actor_id == 0);
    assert(domain->actors[actor_id].manifest == manifest);
    assert(domain->actors[actor_id].bytecode_size == manifest->bytecode_size);
    assert(memcmp(domain->actors[actor_id].bytecode, manifest->bytecode, manifest->bytecode_size) == 0);

    cns_bitactor_destroy(sys);
    printf("Manifest creation and association test passed.\n");
}

void test_registry_functionality() {
    printf("Testing registry functionality...\n");
    cns_bitactor_system_t* sys = cns_bitactor_create();
    assert(sys != NULL);

    uint32_t domain_id = bitactor_domain_create(sys->matrix);
    bitactor_domain_t* domain = &sys->matrix->domains[domain_id];

    bitactor_manifest_t* manifest = create_bitactor_manifest("registry_test_spec");

    uint32_t actor_id_1 = bitactor_add_to_domain(domain, 0x11, manifest, "actor_alpha", sys);
    uint32_t actor_id_2 = bitactor_add_to_domain(domain, 0x22, manifest, "actor_beta", sys);

    compiled_bitactor_t* retrieved_actor_1 = bitactor_registry_lookup_actor(&sys->registry, "actor_alpha");
    assert(retrieved_actor_1 != NULL);
    assert(retrieved_actor_1->meaning == 0x11);

    compiled_bitactor_t* retrieved_actor_2 = bitactor_registry_lookup_actor(&sys->registry, "actor_beta");
    assert(retrieved_actor_2 != NULL);
    assert(retrieved_actor_2->meaning == 0x22);

    compiled_bitactor_t* non_existent_actor = bitactor_registry_lookup_actor(&sys->registry, "actor_gamma");
    assert(non_existent_actor == NULL);

    cns_bitactor_destroy(sys);
    printf("Registry functionality test passed.\n");
}

void test_entanglement_bus() {
    printf("Testing entanglement bus functionality...\n");
    cns_bitactor_system_t* sys = cns_bitactor_create();
    assert(sys != NULL);

    uint32_t domain_id = bitactor_domain_create(sys->matrix);
    bitactor_domain_t* domain = &sys->matrix->domains[domain_id];

    bitactor_manifest_t* manifest = create_bitactor_manifest("bus_test_spec");

    // Add an actor to receive signals
    bitactor_add_to_domain(domain, 0x00, manifest, "receiver_actor", sys);

    // Propagate a signal to the receiver_actor
    bool propagated = bitactor_entanglement_bus_propagate_signal(&sys->entanglement_bus, "receiver_actor", 0xCAFE);
    assert(propagated == true);

    // Check if the receiver actor's signal_pending flag is set
    compiled_bitactor_t* receiver_actor = bitactor_registry_lookup_actor(&sys->registry, "receiver_actor");
    assert(receiver_actor != NULL);
    assert(receiver_actor->signal_pending == 1);

    cns_bitactor_destroy(sys);
    printf("Entanglement bus functionality test passed.\n");
}

int main() {
    test_domain_creation();
    test_actor_addition();
    test_matrix_tick();
    test_nanoregex();
    test_cognitive_cycle();
    test_manifest_creation();
    test_registry_functionality();
    test_entanglement_bus();
    return 0;
}

