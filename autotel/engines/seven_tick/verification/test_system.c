#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <time.h>
#include "../runtime/src/seven_t_runtime.h"

typedef struct {
    uint32_t patient_id;
    uint32_t appointment_id; 
    uint32_t doctor_id;
} QueryResult;

int main() {
    printf("7T System Test\n");
    printf("==============\n\n");
    
    // Create engine
    EngineState* engine = s7t_create_engine();
    
    // Add some test data
    printf("Adding test triples...\n");
    
    // Intern strings for predicates and classes
    uint32_t pred_type = s7t_intern_string(engine, "rdf:type");
    uint32_t pred_hasAppointment = s7t_intern_string(engine, "hasAppointment");
    uint32_t pred_attendedBy = s7t_intern_string(engine, "attendedBy");
    uint32_t pred_hasName = s7t_intern_string(engine, "hasName");
    
    uint32_t class_Patient = s7t_intern_string(engine, "Patient");
    uint32_t class_Doctor = s7t_intern_string(engine, "Doctor");
    uint32_t class_Appointment = s7t_intern_string(engine, "Appointment");
    
    // Create entities
    uint32_t patient1 = s7t_intern_string(engine, "patient1");
    uint32_t patient2 = s7t_intern_string(engine, "patient2");
    uint32_t doctor1 = s7t_intern_string(engine, "doctor1");
    uint32_t appt1 = s7t_intern_string(engine, "appointment1");
    uint32_t appt2 = s7t_intern_string(engine, "appointment2");
    
    // Add type triples
    s7t_add_triple(engine, patient1, pred_type, class_Patient);
    s7t_add_triple(engine, patient2, pred_type, class_Patient);
    s7t_add_triple(engine, doctor1, pred_type, class_Doctor);
    s7t_add_triple(engine, appt1, pred_type, class_Appointment);
    s7t_add_triple(engine, appt2, pred_type, class_Appointment);
    
    // Add relationships
    s7t_add_triple(engine, patient1, pred_hasAppointment, appt1);
    s7t_add_triple(engine, patient2, pred_hasAppointment, appt2);
    s7t_add_triple(engine, appt1, pred_attendedBy, doctor1);
    s7t_add_triple(engine, appt2, pred_attendedBy, doctor1);
    
    // Add names
    uint32_t name1 = s7t_intern_string(engine, "John Doe");
    uint32_t name2 = s7t_intern_string(engine, "Jane Smith");
    uint32_t docname = s7t_intern_string(engine, "Dr. House");
    
    s7t_add_triple(engine, patient1, pred_hasName, name1);
    s7t_add_triple(engine, patient2, pred_hasName, name2);
    s7t_add_triple(engine, doctor1, pred_hasName, docname);
    
    printf("Added %zu triples\n", engine->triple_count);
    
    // Test query primitives
    printf("\nTesting query primitives...\n");
    
    // Find all patients
    BitVector* patients = s7t_get_subject_vector(engine, pred_type, class_Patient);
    printf("Found %zu patients\n", patients->count);
    
    // Test SHACL validation
    printf("\nTesting SHACL validation...\n");
    int valid1 = shacl_check_min_count(engine, patient1, pred_hasAppointment, 1);
    int valid2 = shacl_check_min_count(engine, patient1, pred_hasName, 1);
    printf("Patient1 has appointment: %s\n", valid1 ? "YES" : "NO");
    printf("Patient1 has name: %s\n", valid2 ? "YES" : "NO");
    
    // Load and test compiled kernel
    printf("\nLoading compiled kernel...\n");
    void* kernel = dlopen("/tmp/kernel.so", RTLD_NOW);
    if (!kernel) {
        fprintf(stderr, "Failed to load kernel: %s\n", dlerror());
        s7t_destroy_engine(engine);
        return 1;
    }
    
    typedef QueryResult* (*query_func)(EngineState*, size_t*);
    query_func execute_query = dlsym(kernel, "execute_query_1");
    if (!execute_query) {
        fprintf(stderr, "Failed to find query function\n");
        dlclose(kernel);
        s7t_destroy_engine(engine);
        return 1;
    }
    
    // Execute query
    size_t result_count;
    QueryResult* results = execute_query(engine, &result_count);
    
    printf("Query returned %zu results\n", result_count);
    for (size_t i = 0; i < result_count; i++) {
        printf("  Patient %u -> Appointment %u -> Doctor %u\n",
               results[i].patient_id, results[i].appointment_id, results[i].doctor_id);
    }
    
    // Performance test
    printf("\nPerformance test (10000 iterations)...\n");
    clock_t start = clock();
    
    for (int i = 0; i < 10000; i++) {
        free(results);
        results = execute_query(engine, &result_count);
    }
    
    clock_t end = clock();
    double cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("Time: %.3f seconds\n", cpu_time);
    printf("Throughput: %.2f queries/second\n", 10000.0 / cpu_time);
    printf("Avg latency: %.2f microseconds\n", (cpu_time * 1e6) / 10000.0);
    
    // Cleanup
    free(results);
    bitvec_destroy(patients);
    dlclose(kernel);
    s7t_destroy_engine(engine);
    
    printf("\nTest completed successfully!\n");
    return 0;
}
