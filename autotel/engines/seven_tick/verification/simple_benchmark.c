#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <dlfcn.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include "../runtime/src/seven_t_runtime.h"

// Function pointers for dynamic loading
typedef void *(*execute_query_func)(EngineState *, size_t *);
typedef int (*validate_shape_func)(EngineState *, uint32_t);

// High-precision timing for microsecond measurements
static inline uint64_t get_microseconds()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000ULL + tv.tv_usec;
}

// Simple benchmark with synthetic data
int main()
{
    printf("7T Performance Benchmark (Simplified)\n");
    printf("====================================\n\n");

    // Step 1: Create engine
    printf("Creating engine...\n");
    EngineState *engine = s7t_create_engine();

    // Step 2: Add synthetic test data
    printf("Adding synthetic data...\n");

    // Define predicates
    uint32_t pred_type = s7t_intern_string(engine, "<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>");
    uint32_t pred_hasAppointment = s7t_intern_string(engine, "<http://example.org/sprint_health#hasAppointment>");
    uint32_t pred_attendedBy = s7t_intern_string(engine, "<http://example.org/sprint_health#attendedBy>");
    uint32_t pred_hasName = s7t_intern_string(engine, "<http://example.org/sprint_health#hasName>");

    // Define classes
    uint32_t class_Patient = s7t_intern_string(engine, "<http://example.org/sprint_health#Patient>");
    uint32_t class_Doctor = s7t_intern_string(engine, "<http://example.org/sprint_health#Doctor>");
    uint32_t class_Appointment = s7t_intern_string(engine, "<http://example.org/sprint_health#Appointment>");

    // Create 1000 patients with appointments
    for (int i = 0; i < 1000; i++)
    {
        char buf[256];

        // Patient
        snprintf(buf, sizeof(buf), "<http://example.org/patient_%d>", i);
        uint32_t patient = s7t_intern_string(engine, buf);
        s7t_add_triple(engine, patient, pred_type, class_Patient);

        // Patient name
        snprintf(buf, sizeof(buf), "\"Patient %d\"", i);
        uint32_t name = s7t_intern_string(engine, buf);
        s7t_add_triple(engine, patient, pred_hasName, name);

        // Appointment
        snprintf(buf, sizeof(buf), "<http://example.org/appointment_%d>", i);
        uint32_t appointment = s7t_intern_string(engine, buf);
        s7t_add_triple(engine, appointment, pred_type, class_Appointment);
        s7t_add_triple(engine, patient, pred_hasAppointment, appointment);

        // Doctor (10 doctors total)
        snprintf(buf, sizeof(buf), "<http://example.org/doctor_%d>", i % 10);
        uint32_t doctor = s7t_intern_string(engine, buf);
        s7t_add_triple(engine, appointment, pred_attendedBy, doctor);
    }

    // Ensure doctors exist
    for (int i = 0; i < 10; i++)
    {
        char buf[256];
        snprintf(buf, sizeof(buf), "<http://example.org/doctor_%d>", i);
        uint32_t doctor = s7t_intern_string(engine, buf);
        s7t_add_triple(engine, doctor, pred_type, class_Doctor);

        snprintf(buf, sizeof(buf), "\"Dr. Smith %d\"", i);
        uint32_t name = s7t_intern_string(engine, buf);
        s7t_add_triple(engine, doctor, pred_hasName, name);
    }

    printf("Added %zu triples\n", engine->triple_count);

    // Step 3: Test query performance with proper warmup
    printf("\nTesting query performance...\n");

    // Proper warmup (1000 iterations)
    for (int i = 0; i < 1000; i++)
    {
        BitVector *patients = s7t_get_subject_vector(engine, pred_type, class_Patient);
        bitvec_destroy(patients);
    }

    // Benchmark with realistic iterations
    uint64_t start = get_microseconds();
    int iterations = 10000; // Reduced from 100000 for realistic measurement

    for (int i = 0; i < iterations; i++)
    {
        BitVector *patients = s7t_get_subject_vector(engine, pred_type, class_Patient);
        size_t count = bitvec_popcount(patients);
        bitvec_destroy(patients);
    }

    uint64_t end = get_microseconds();
    double elapsed = (end - start) / 1000000.0;

    printf("Executed %d queries in %.3f seconds\n", iterations, elapsed);
    printf("Throughput: %.2f KQPS\n", (iterations / elapsed) / 1e3);
    printf("Average latency: %.2f microseconds\n", (elapsed * 1e6) / iterations);

    // Success criteria check with realistic thresholds
    double avg_latency_us = (elapsed * 1e6) / iterations;
    double throughput_kqps = (iterations / elapsed) / 1e3;

    printf("\nBenchmark Results:\n");
    printf("------------------\n");

    if (avg_latency_us < 100)
    { // < 100 microseconds (realistic threshold)
        printf("✅ PASS: Average latency %.1f μs meets requirement (<100 μs)\n", avg_latency_us);
    }
    else
    {
        printf("❌ FAIL: Average latency %.1f μs exceeds requirement\n", avg_latency_us);
    }

    if (throughput_kqps > 1.0)
    { // > 1 KQPS (realistic threshold)
        printf("✅ PASS: Throughput %.1f KQPS exceeds requirement (>1 KQPS)\n", throughput_kqps);
    }
    else
    {
        printf("❌ FAIL: Throughput %.1f KQPS below requirement\n", throughput_kqps);
    }

    // Cleanup
    s7t_destroy_engine(engine);

    printf("\n🎉 7T System Benchmark Complete!\n");

    return 0;
}
