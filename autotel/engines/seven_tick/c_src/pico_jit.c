#include "pico_jit.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/wait.h>

// Shape frequency tracking
typedef struct ShapeStats {
    QueryShape shape;
    uint32_t exec_count;
    uint64_t total_cycles;
    struct ShapeStats* next;
} ShapeStats;

// Compile job
typedef struct CompileJob {
    QueryShape shape;
    char source_path[256];
    char lib_path[256];
    struct CompileJob* next;
} CompileJob;

// JIT daemon state
struct JitDaemon {
    JitConfig config;
    
    // Shape tracking
    ShapeStats* shape_buckets[1024];
    pthread_mutex_t shape_mutex;
    
    // Kernel cache (LRU)
    JitKernel* kernels;
    size_t kernel_count;
    size_t kernel_capacity;
    size_t total_code_bytes;
    pthread_rwlock_t cache_lock;
    
    // Compile queue
    CompileJob* compile_queue_head;
    CompileJob* compile_queue_tail;
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_cond;
    
    // Worker threads
    pthread_t* workers;
    bool shutdown;
    
    // Telemetry
    jit_telemetry_fn telemetry;
    
    // Stats
    uint64_t kernels_compiled;
    uint64_t compile_failures;
    uint64_t cache_hits;
    uint64_t cache_misses;
};

// Hash query shape
static uint64_t hash_shape(const QueryShape* shape) {
    uint64_t hash = 14695981039346656037ULL;
    for (int i = 0; i < shape->pattern_count; i++) {
        hash ^= shape->pattern_ids[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

// Generate C code for shape
static void generate_kernel_code(const QueryShape* shape, const char* output_path) {
    FILE* f = fopen(output_path, "w");
    if (!f) abort();
    
    fprintf(f, "// Auto-generated JIT kernel\n");
    fprintf(f, "#include <stdint.h>\n");
    fprintf(f, "#include <stdbool.h>\n\n");
    
    fprintf(f, "__attribute__((always_inline))\n");
    fprintf(f, "bool jit_kernel_%016llx(uint64_t* pred_vecs, uint64_t* obj_vecs,\n", shape->hash);
    fprintf(f, "                        uint32_t* params, uint64_t* result) {\n");
    
    // Generate unrolled bit operations
    fprintf(f, "    // Unrolled %d-pattern query\n", shape->pattern_count);
    fprintf(f, "    uint64_t accum = ~0ULL;\n");
    
    for (int i = 0; i < shape->pattern_count; i++) {
        uint32_t pattern = shape->pattern_ids[i];
        
        // Each pattern: pred_vec[pattern] & obj_vec[params[i]]
        fprintf(f, "    // Pattern %d (id=%u)\n", i, pattern);
        fprintf(f, "    {\n");
        fprintf(f, "        uint32_t obj_id = params[%d];\n", i);
        fprintf(f, "        uint64_t p = pred_vecs[%u];\n", pattern);
        fprintf(f, "        uint64_t o = obj_vecs[obj_id];\n");
        fprintf(f, "        accum &= (p & o);\n");
        fprintf(f, "        if (!accum) return false; // Early exit\n");
        fprintf(f, "    }\n");
    }
    
    fprintf(f, "    *result = accum;\n");
    fprintf(f, "    return accum != 0;\n");
    fprintf(f, "}\n\n");
    
    // Export symbol
    fprintf(f, "// Export for dlsym\n");
    fprintf(f, "jit_kernel_fn get_kernel() {\n");
    fprintf(f, "    return (jit_kernel_fn)jit_kernel_%016llx;\n", shape->hash);
    fprintf(f, "}\n");
    
    fclose(f);
}

// Get hardware ticks helper
static inline uint64_t get_hardware_ticks(void) {
#if defined(__aarch64__)
    uint64_t val;
    __asm__ __volatile__ ("mrs %0, cntvct_el0" : "=r" (val));
    return val;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#endif
}

// Compile worker thread
static void* compile_worker(void* arg) {
    JitDaemon* daemon = (JitDaemon*)arg;
    
    while (!daemon->shutdown) {
        // Get next compile job
        pthread_mutex_lock(&daemon->queue_mutex);
        while (!daemon->compile_queue_head && !daemon->shutdown) {
            pthread_cond_wait(&daemon->queue_cond, &daemon->queue_mutex);
        }
        
        if (daemon->shutdown) {
            pthread_mutex_unlock(&daemon->queue_mutex);
            break;
        }
        
        CompileJob* job = daemon->compile_queue_head;
        daemon->compile_queue_head = job->next;
        if (!daemon->compile_queue_head) {
            daemon->compile_queue_tail = NULL;
        }
        pthread_mutex_unlock(&daemon->queue_mutex);
        
        // Compile the kernel
        uint64_t compile_start = get_hardware_ticks();
        
        if (daemon->telemetry) {
            daemon->telemetry("jit.compile.start", 1.0);
        }
        
        // Generate C code
        generate_kernel_code(&job->shape, job->source_path);
        
        // Fork compiler (safer than system())
        pid_t pid = fork();
        if (pid == 0) {
            // Child: exec compiler
            char* argv[] = {
                "gcc",
                "-O3", "-march=native", "-fPIC", "-shared",
                "-DPICO_DISABLE_CONTRACTS",
                "-o", job->lib_path,
                job->source_path,
                NULL
            };
            
            execvp("gcc", argv);
            exit(1); // Exec failed
        }
        
        // Parent: wait for compiler
        int status;
        waitpid(pid, &status, 0);
        
        uint64_t compile_cycles = get_hardware_ticks() - compile_start;
        
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            // Success - load the kernel
            void* handle = dlopen(job->lib_path, RTLD_NOW | RTLD_LOCAL);
            if (handle) {
                typedef jit_kernel_fn (*get_kernel_fn)(void);
                get_kernel_fn get_kernel = dlsym(handle, "get_kernel");
                
                if (get_kernel) {
                    jit_kernel_fn kernel = get_kernel();
                    
                    // Add to cache
                    pthread_rwlock_wrlock(&daemon->cache_lock);
                    
                    if (daemon->kernel_count >= daemon->kernel_capacity) {
                        // Evict LRU (simplified: just evict first)
                        if (daemon->kernels[0].handle) {
                            dlclose(daemon->kernels[0].handle);
                            daemon->total_code_bytes -= 4096; // Estimate
                        }
                        
                        // Shift array
                        memmove(&daemon->kernels[0], &daemon->kernels[1],
                               (daemon->kernel_count - 1) * sizeof(JitKernel));
                        daemon->kernel_count--;
                    }
                    
                    // Add new kernel
                    JitKernel* entry = &daemon->kernels[daemon->kernel_count++];
                    entry->shape = job->shape;
                    entry->kernel = kernel;
                    entry->handle = handle;
                    entry->compile_cycles = compile_cycles;
                    entry->hit_count = 0;
                    entry->total_cycles_saved = 0;
                    
                    daemon->total_code_bytes += 4096; // Estimate
                    daemon->kernels_compiled++;
                    
                    pthread_rwlock_unlock(&daemon->cache_lock);
                    
                    if (daemon->telemetry) {
                        daemon->telemetry("jit.compile.success", compile_cycles);
                    }
                } else {
                    dlclose(handle);
                    daemon->compile_failures++;
                }
            } else {
                daemon->compile_failures++;
            }
        } else {
            daemon->compile_failures++;
            if (daemon->telemetry) {
                daemon->telemetry("jit.compile.failure", 1.0);
            }
        }
        
        // Cleanup
        unlink(job->source_path);
        unlink(job->lib_path);
        free(job);
    }
    
    return NULL;
}

// Create JIT daemon
JitDaemon* jit_create(const JitConfig* config) {
    JitDaemon* daemon = calloc(1, sizeof(JitDaemon));
    daemon->config = *config;
    
    // Initialize shape tracking
    pthread_mutex_init(&daemon->shape_mutex, NULL);
    
    // Initialize kernel cache
    daemon->kernel_capacity = config->max_cache_size;
    daemon->kernels = calloc(daemon->kernel_capacity, sizeof(JitKernel));
    pthread_rwlock_init(&daemon->cache_lock, NULL);
    
    // Initialize compile queue
    pthread_mutex_init(&daemon->queue_mutex, NULL);
    pthread_cond_init(&daemon->queue_cond, NULL);
    
    // Start worker threads
    daemon->workers = calloc(config->compile_threads, sizeof(pthread_t));
    for (uint32_t i = 0; i < config->compile_threads; i++) {
        pthread_create(&daemon->workers[i], NULL, compile_worker, daemon);
    }
    
    return daemon;
}

// Record execution
void jit_record_execution(JitDaemon* daemon, const QueryShape* shape, uint64_t cycles) {
    uint64_t hash = hash_shape(shape);
    uint32_t bucket = hash % 1024;
    
    pthread_mutex_lock(&daemon->shape_mutex);
    
    // Find or create stats entry
    ShapeStats* stats = daemon->shape_buckets[bucket];
    while (stats && stats->shape.hash != hash) {
        stats = stats->next;
    }
    
    if (!stats) {
        stats = calloc(1, sizeof(ShapeStats));
        stats->shape = *shape;
        stats->shape.hash = hash;
        stats->next = daemon->shape_buckets[bucket];
        daemon->shape_buckets[bucket] = stats;
    }
    
    stats->exec_count++;
    stats->total_cycles += cycles;
    
    // Check if hot
    if (stats->exec_count == daemon->config.hot_threshold) {
        // Schedule compilation
        CompileJob* job = calloc(1, sizeof(CompileJob));
        job->shape = *shape;
        job->shape.hash = hash;
        
        snprintf(job->source_path, sizeof(job->source_path),
                "/tmp/jit_%016llx.c", hash);
        snprintf(job->lib_path, sizeof(job->lib_path),
                "/tmp/jit_%016llx.so", hash);
        
        pthread_mutex_lock(&daemon->queue_mutex);
        if (daemon->compile_queue_tail) {
            daemon->compile_queue_tail->next = job;
            daemon->compile_queue_tail = job;
        } else {
            daemon->compile_queue_head = daemon->compile_queue_tail = job;
        }
        pthread_cond_signal(&daemon->queue_cond);
        pthread_mutex_unlock(&daemon->queue_mutex);
        
        if (daemon->telemetry) {
            daemon->telemetry("jit.hot_shape_detected", stats->exec_count);
        }
    }
    
    pthread_mutex_unlock(&daemon->shape_mutex);
}

// Lookup kernel
jit_kernel_fn jit_lookup_kernel(JitDaemon* daemon, const QueryShape* shape) {
    uint64_t hash = hash_shape(shape);
    
    pthread_rwlock_rdlock(&daemon->cache_lock);
    
    // Linear search (could use hash table)
    for (size_t i = 0; i < daemon->kernel_count; i++) {
        if (daemon->kernels[i].shape.hash == hash) {
            daemon->kernels[i].hit_count++;
            daemon->cache_hits++;
            jit_kernel_fn kernel = daemon->kernels[i].kernel;
            pthread_rwlock_unlock(&daemon->cache_lock);
            
            if (daemon->telemetry) {
                daemon->telemetry("jit.cache_hit", 1.0);
            }
            
            return kernel;
        }
    }
    
    daemon->cache_misses++;
    pthread_rwlock_unlock(&daemon->cache_lock);
    
    return NULL;
}

// Get stats
void jit_get_stats(JitDaemon* daemon, char* buf, size_t buf_size) {
    snprintf(buf, buf_size,
            "JIT Stats:\n"
            "  Kernels compiled: %llu\n"
            "  Compile failures: %llu\n"
            "  Cache hits: %llu\n"
            "  Cache misses: %llu\n"
            "  Total code bytes: %zu\n"
            "  Kernels cached: %zu/%zu\n",
            daemon->kernels_compiled,
            daemon->compile_failures,
            daemon->cache_hits,
            daemon->cache_misses,
            daemon->total_code_bytes,
            daemon->kernel_count,
            daemon->kernel_capacity);
}

// Cleanup
void jit_destroy(JitDaemon* daemon) {
    // Signal shutdown
    daemon->shutdown = true;
    pthread_cond_broadcast(&daemon->queue_cond);
    
    // Wait for workers
    for (uint32_t i = 0; i < daemon->config.compile_threads; i++) {
        pthread_join(daemon->workers[i], NULL);
    }
    
    // Unload kernels
    for (size_t i = 0; i < daemon->kernel_count; i++) {
        if (daemon->kernels[i].handle) {
            dlclose(daemon->kernels[i].handle);
        }
    }
    
    // Free memory
    free(daemon->kernels);
    free(daemon->workers);
    
    // Free shape stats
    for (int i = 0; i < 1024; i++) {
        ShapeStats* stats = daemon->shape_buckets[i];
        while (stats) {
            ShapeStats* next = stats->next;
            free(stats);
            stats = next;
        }
    }
    
    free(daemon);
}

// Set telemetry
void jit_set_telemetry(JitDaemon* daemon, jit_telemetry_fn fn) {
    daemon->telemetry = fn;
}
