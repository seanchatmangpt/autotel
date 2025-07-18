# Pattern: Concurrent Access and Thread Safety

## Description
This pattern demonstrates how to safely use the 7T Engine in multi-threaded environments, including thread-safe operations, read-write locks, and concurrent access patterns for high-performance applications.

## Code Example
```c
#include "c_src/sparql7t.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

// Thread-safe engine wrapper
typedef struct {
    S7TEngine* engine;
    pthread_rwlock_t rwlock;
    int reader_count;
    pthread_mutex_t reader_mutex;
} ThreadSafeEngine;

// Initialize thread-safe engine
ThreadSafeEngine* ts_engine_create(size_t max_s, size_t max_p, size_t max_o) {
    ThreadSafeEngine* ts_engine = malloc(sizeof(ThreadSafeEngine));
    if (!ts_engine) return NULL;
    
    ts_engine->engine = s7t_create(max_s, max_p, max_o);
    if (!ts_engine->engine) {
        free(ts_engine);
        return NULL;
    }
    
    pthread_rwlock_init(&ts_engine->rwlock, NULL);
    pthread_mutex_init(&ts_engine->reader_mutex, NULL);
    ts_engine->reader_count = 0;
    
    return ts_engine;
}

// Destroy thread-safe engine
void ts_engine_destroy(ThreadSafeEngine* ts_engine) {
    if (!ts_engine) return;
    
    s7t_destroy(ts_engine->engine);
    pthread_rwlock_destroy(&ts_engine->rwlock);
    pthread_mutex_destroy(&ts_engine->reader_mutex);
    free(ts_engine);
}

// Thread-safe triple addition (write operation)
void ts_add_triple(ThreadSafeEngine* ts_engine, uint32_t s, uint32_t p, uint32_t o) {
    pthread_rwlock_wrlock(&ts_engine->rwlock);
    s7t_add_triple(ts_engine->engine, s, p, o);
    pthread_rwlock_unlock(&ts_engine->rwlock);
}

// Thread-safe pattern matching (read operation)
int ts_ask_pattern(ThreadSafeEngine* ts_engine, uint32_t s, uint32_t p, uint32_t o) {
    pthread_rwlock_rdlock(&ts_engine->rwlock);
    int result = s7t_ask_pattern(ts_engine->engine, s, p, o);
    pthread_rwlock_unlock(&ts_engine->rwlock);
    return result;
}

// Thread-safe batch operations
void ts_ask_batch(ThreadSafeEngine* ts_engine, TriplePattern* patterns, int* results, size_t count) {
    pthread_rwlock_rdlock(&ts_engine->rwlock);
    s7t_ask_batch(ts_engine->engine, patterns, results, count);
    pthread_rwlock_unlock(&ts_engine->rwlock);
}

// Optimistic reader pattern
typedef struct {
    ThreadSafeEngine* ts_engine;
    int thread_id;
    int operations;
} ReaderThread;

void* reader_thread_func(void* arg) {
    ReaderThread* reader = (ReaderThread*)arg;
    ThreadSafeEngine* ts_engine = reader->ts_engine;
    
    printf("Reader thread %d starting\n", reader->thread_id);
    
    for (int i = 0; i < reader->operations; i++) {
        uint32_t s = (reader->thread_id * 1000 + i) % ts_engine->engine->max_subjects;
        uint32_t p = i % ts_engine->engine->max_predicates;
        uint32_t o = i % ts_engine->engine->max_objects;
        
        int result = ts_ask_pattern(ts_engine, s, p, o);
        
        if (i % 1000 == 0) {
            printf("Reader %d: completed %d operations\n", reader->thread_id, i);
        }
    }
    
    printf("Reader thread %d completed\n", reader->thread_id);
    return NULL;
}

// Writer thread pattern
typedef struct {
    ThreadSafeEngine* ts_engine;
    int thread_id;
    int operations;
} WriterThread;

void* writer_thread_func(void* arg) {
    WriterThread* writer = (WriterThread*)arg;
    ThreadSafeEngine* ts_engine = writer->ts_engine;
    
    printf("Writer thread %d starting\n", writer->thread_id);
    
    for (int i = 0; i < writer->operations; i++) {
        uint32_t s = (writer->thread_id * 1000 + i) % ts_engine->engine->max_subjects;
        uint32_t p = i % ts_engine->engine->max_predicates;
        uint32_t o = i % ts_engine->engine->max_objects;
        
        ts_add_triple(ts_engine, s, p, o);
        
        if (i % 1000 == 0) {
            printf("Writer %d: completed %d operations\n", writer->thread_id, i);
        }
    }
    
    printf("Writer thread %d completed\n", writer->thread_id);
    return NULL;
}

// Concurrent batch operations
typedef struct {
    ThreadSafeEngine* ts_engine;
    int thread_id;
    int batch_size;
    int num_batches;
} BatchThread;

void* batch_thread_func(void* arg) {
    BatchThread* batch = (BatchThread*)arg;
    ThreadSafeEngine* ts_engine = batch->ts_engine;
    
    TriplePattern* patterns = malloc(batch->batch_size * sizeof(TriplePattern));
    int* results = malloc(batch->batch_size * sizeof(int));
    
    if (!patterns || !results) {
        free(patterns);
        free(results);
        return NULL;
    }
    
    printf("Batch thread %d starting\n", batch->thread_id);
    
    for (int batch_num = 0; batch_num < batch->num_batches; batch_num++) {
        // Prepare batch patterns
        for (int i = 0; i < batch->batch_size; i++) {
            patterns[i].s = (batch->thread_id * 1000 + batch_num * batch->batch_size + i) % ts_engine->engine->max_subjects;
            patterns[i].p = i % ts_engine->engine->max_predicates;
            patterns[i].o = i % ts_engine->engine->max_objects;
        }
        
        // Execute batch operation
        ts_ask_batch(ts_engine, patterns, results, batch->batch_size);
        
        if (batch_num % 10 == 0) {
            printf("Batch thread %d: completed %d batches\n", batch->thread_id, batch_num);
        }
    }
    
    free(patterns);
    free(results);
    printf("Batch thread %d completed\n", batch->thread_id);
    return NULL;
}

// Performance measurement
typedef struct {
    clock_t start_time;
    clock_t end_time;
    int thread_id;
    char operation_type[32];
} PerformanceRecord;

void record_performance(PerformanceRecord* record, int thread_id, const char* operation_type) {
    record->thread_id = thread_id;
    strncpy(record->operation_type, operation_type, 31);
    record->operation_type[31] = '\0';
    record->start_time = clock();
}

void finish_performance(PerformanceRecord* record) {
    record->end_time = clock();
    double seconds = (double)(record->end_time - record->start_time) / CLOCKS_PER_SEC;
    printf("Thread %d (%s): %.3f seconds\n", record->thread_id, record->operation_type, seconds);
}

// Concurrent access test
int test_concurrent_access() {
    const int num_readers = 4;
    const int num_writers = 2;
    const int num_batch_threads = 2;
    const int operations_per_thread = 10000;
    const int batch_size = 100;
    const int num_batches = 100;
    
    ThreadSafeEngine* ts_engine = ts_engine_create(100000, 1000, 100000);
    if (!ts_engine) {
        fprintf(stderr, "Failed to create thread-safe engine\n");
        return 1;
    }
    
    pthread_t* reader_threads = malloc(num_readers * sizeof(pthread_t));
    pthread_t* writer_threads = malloc(num_writers * sizeof(pthread_t));
    pthread_t* batch_threads = malloc(num_batch_threads * sizeof(pthread_t));
    
    ReaderThread* readers = malloc(num_readers * sizeof(ReaderThread));
    WriterThread* writers = malloc(num_writers * sizeof(WriterThread));
    BatchThread* batches = malloc(num_batch_threads * sizeof(BatchThread));
    
    if (!reader_threads || !writer_threads || !batch_threads || 
        !readers || !writers || !batches) {
        fprintf(stderr, "Failed to allocate thread structures\n");
        return 1;
    }
    
    // Initialize thread parameters
    for (int i = 0; i < num_readers; i++) {
        readers[i].ts_engine = ts_engine;
        readers[i].thread_id = i;
        readers[i].operations = operations_per_thread;
    }
    
    for (int i = 0; i < num_writers; i++) {
        writers[i].ts_engine = ts_engine;
        writers[i].thread_id = i;
        writers[i].operations = operations_per_thread;
    }
    
    for (int i = 0; i < num_batch_threads; i++) {
        batches[i].ts_engine = ts_engine;
        batches[i].thread_id = i;
        batches[i].batch_size = batch_size;
        batches[i].num_batches = num_batches;
    }
    
    printf("Starting concurrent access test with %d readers, %d writers, %d batch threads\n",
           num_readers, num_writers, num_batch_threads);
    
    clock_t start_time = clock();
    
    // Start all threads
    for (int i = 0; i < num_readers; i++) {
        pthread_create(&reader_threads[i], NULL, reader_thread_func, &readers[i]);
    }
    
    for (int i = 0; i < num_writers; i++) {
        pthread_create(&writer_threads[i], NULL, writer_thread_func, &writers[i]);
    }
    
    for (int i = 0; i < num_batch_threads; i++) {
        pthread_create(&batch_threads[i], NULL, batch_thread_func, &batches[i]);
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < num_readers; i++) {
        pthread_join(reader_threads[i], NULL);
    }
    
    for (int i = 0; i < num_writers; i++) {
        pthread_join(writer_threads[i], NULL);
    }
    
    for (int i = 0; i < num_batch_threads; i++) {
        pthread_join(batch_threads[i], NULL);
    }
    
    clock_t end_time = clock();
    double total_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    printf("Concurrent access test completed in %.3f seconds\n", total_time);
    printf("Total operations: %d\n", 
           num_readers * operations_per_thread + 
           num_writers * operations_per_thread + 
           num_batch_threads * num_batches * batch_size);
    
    // Cleanup
    free(reader_threads);
    free(writer_threads);
    free(batch_threads);
    free(readers);
    free(writers);
    free(batches);
    ts_engine_destroy(ts_engine);
    
    return 0;
}

int main() {
    return test_concurrent_access();
}
```

## Tips
- Use read-write locks for better concurrency (multiple readers, single writer).
- Consider lock-free data structures for extreme performance.
- Monitor thread contention and adjust lock granularity.
- Use thread pools for batch operations.
- Profile with tools like `perf` and `valgrind --tool=helgrind`. 