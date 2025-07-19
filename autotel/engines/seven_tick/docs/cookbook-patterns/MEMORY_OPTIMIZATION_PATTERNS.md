# Memory Optimization Patterns Cookbook

## Overview

This cookbook provides practical patterns and examples for optimizing memory usage in the 7T engine to achieve **7-tick performance** (< 10 nanoseconds) while maintaining efficient memory management.

## Table of Contents

1. [Memory Allocation Patterns](#memory-allocation-patterns)
2. [Cache-Aware Data Structures](#cache-aware-data-structures)
3. [Memory Pooling Patterns](#memory-pooling-patterns)
4. [String Optimization Patterns](#string-optimization-patterns)
5. [Memory Hierarchy Optimization](#memory-hierarchy-optimization)
6. [Zero-Copy Patterns](#zero-copy-patterns)
7. [Memory Profiling Patterns](#memory-profiling-patterns)

## Memory Allocation Patterns

### Pattern 1: Stack Allocation for Small Objects

**Use Case**: Allocate small, short-lived objects on the stack for maximum performance.

```c
#include <stdint.h>
#include <string.h>

// L1 cache-friendly structure (64 bytes)
typedef struct {
    uint32_t data[16];  // 64 bytes total
    uint32_t count;
} L1OptimizedBuffer;

// L2 cache-friendly structure (256 bytes)
typedef struct {
    uint32_t data[64];  // 256 bytes total
    uint32_t count;
} L2OptimizedBuffer;

void process_small_data_stack() {
    // Allocate on stack - no heap allocation overhead
    L1OptimizedBuffer buffer = {0};
    
    // Fill buffer
    for (int i = 0; i < 16; i++) {
        buffer.data[i] = i;
    }
    buffer.count = 16;
    
    // Process data
    uint32_t sum = 0;
    for (int i = 0; i < buffer.count; i++) {
        sum += buffer.data[i];
    }
    
    // Automatic cleanup when function returns
    printf("Sum: %u\n", sum);
}
```

### Pattern 2: Arena Allocation

**Use Case**: Allocate multiple objects from a single memory arena for better locality.

```c
typedef struct {
    char* memory;
    size_t size;
    size_t used;
} MemoryArena;

MemoryArena* create_arena(size_t size) {
    MemoryArena* arena = malloc(sizeof(MemoryArena));
    arena->memory = malloc(size);
    arena->size = size;
    arena->used = 0;
    return arena;
}

void* arena_alloc(MemoryArena* arena, size_t size) {
    if (arena->used + size > arena->size) {
        return NULL;  // Arena full
    }
    
    void* ptr = arena->memory + arena->used;
    arena->used += size;
    return ptr;
}

void destroy_arena(MemoryArena* arena) {
    free(arena->memory);
    free(arena);
}

void example_arena_usage() {
    MemoryArena* arena = create_arena(1024 * 1024);  // 1MB arena
    
    // Allocate multiple objects from arena
    uint32_t* numbers = arena_alloc(arena, 100 * sizeof(uint32_t));
    char* strings = arena_alloc(arena, 50 * 64);  // 50 strings of 64 bytes each
    
    // Use allocated memory
    for (int i = 0; i < 100; i++) {
        numbers[i] = i;
    }
    
    // All objects freed when arena is destroyed
    destroy_arena(arena);
}
```

## Cache-Aware Data Structures

### Pattern 3: Cache-Line Aligned Structures

**Use Case**: Align data structures to cache lines for optimal access patterns.

```c
#include <stdalign.h>

// Cache line size (typically 64 bytes)
#define CACHE_LINE_SIZE 64

// Structure aligned to cache line
typedef struct alignas(CACHE_LINE_SIZE) {
    uint32_t id;
    uint32_t data[15];  // 60 bytes of data
    // 4 bytes padding to reach 64 bytes
} CacheAlignedStruct;

// Array of cache-aligned structures
typedef struct {
    CacheAlignedStruct* data;
    size_t count;
    size_t capacity;
} CacheAlignedArray;

CacheAlignedArray* create_cache_aligned_array(size_t initial_capacity) {
    CacheAlignedArray* array = malloc(sizeof(CacheAlignedArray));
    array->data = aligned_alloc(CACHE_LINE_SIZE, 
                               initial_capacity * sizeof(CacheAlignedStruct));
    array->count = 0;
    array->capacity = initial_capacity;
    return array;
}

void destroy_cache_aligned_array(CacheAlignedArray* array) {
    free(array->data);
    free(array);
}
```

### Pattern 4: Hot/Cold Data Separation

**Use Case**: Separate frequently accessed (hot) data from rarely accessed (cold) data.

```c
// Hot data - accessed frequently, kept in L1 cache
typedef struct {
    uint32_t id;
    uint32_t status;
    uint32_t priority;
    uint32_t timestamp;
} HotData;

// Cold data - accessed rarely, can be in slower memory
typedef struct {
    char description[256];
    char metadata[512];
    uint32_t history[100];
} ColdData;

// Combined structure with hot data first
typedef struct {
    HotData hot;    // 16 bytes - fits in L1 cache
    ColdData cold;  // 868 bytes - can be in L2/L3 cache
} DataRecord;

void process_hot_data(DataRecord* records, size_t count) {
    // Process only hot data for maximum performance
    for (size_t i = 0; i < count; i++) {
        HotData* hot = &records[i].hot;
        
        // Fast access to hot data
        if (hot->status == 1 && hot->priority > 5) {
            hot->timestamp = get_current_time();
        }
    }
}
```

## Memory Pooling Patterns

### Pattern 5: Fixed-Size Memory Pool

**Use Case**: Pre-allocate fixed-size blocks for fast allocation/deallocation.

```c
typedef struct {
    void* blocks[1024];
    size_t free_count;
    size_t block_size;
} FixedSizePool;

FixedSizePool* create_fixed_pool(size_t block_size, size_t block_count) {
    FixedSizePool* pool = malloc(sizeof(FixedSizePool));
    pool->block_size = block_size;
    pool->free_count = block_count;
    
    // Pre-allocate all blocks
    for (size_t i = 0; i < block_count; i++) {
        pool->blocks[i] = malloc(block_size);
    }
    
    return pool;
}

void* pool_alloc(FixedSizePool* pool) {
    if (pool->free_count == 0) {
        return NULL;  // Pool exhausted
    }
    
    return pool->blocks[--pool->free_count];
}

void pool_free(FixedSizePool* pool, void* ptr) {
    if (pool->free_count < 1024) {
        pool->blocks[pool->free_count++] = ptr;
    }
    // If pool is full, ptr is effectively leaked
    // In practice, you might want to free it
}

void destroy_fixed_pool(FixedSizePool* pool) {
    for (size_t i = 0; i < 1024; i++) {
        free(pool->blocks[i]);
    }
    free(pool);
}
```

### Pattern 6: Object Pool with Reuse

**Use Case**: Reuse objects to avoid allocation/deallocation overhead.

```c
typedef struct {
    uint32_t id;
    uint32_t data[16];
    int in_use;
} PooledObject;

typedef struct {
    PooledObject* objects;
    size_t count;
    size_t capacity;
} ObjectPool;

ObjectPool* create_object_pool(size_t capacity) {
    ObjectPool* pool = malloc(sizeof(ObjectPool));
    pool->objects = calloc(capacity, sizeof(PooledObject));
    pool->count = 0;
    pool->capacity = capacity;
    return pool;
}

PooledObject* pool_get_object(ObjectPool* pool) {
    // Find unused object
    for (size_t i = 0; i < pool->capacity; i++) {
        if (!pool->objects[i].in_use) {
            pool->objects[i].in_use = 1;
            pool->count++;
            return &pool->objects[i];
        }
    }
    return NULL;  // No free objects
}

void pool_return_object(ObjectPool* pool, PooledObject* obj) {
    obj->in_use = 0;
    pool->count--;
}

void destroy_object_pool(ObjectPool* pool) {
    free(pool->objects);
    free(pool);
}
```

## String Optimization Patterns

### Pattern 7: String Interning

**Use Case**: Eliminate duplicate string storage and enable O(1) string comparison.

```c
typedef struct {
    char* string;
    uint32_t id;
} StringEntry;

typedef struct {
    StringEntry* entries;
    size_t count;
    size_t capacity;
} StringInterner;

StringInterner* create_string_interner(size_t initial_capacity) {
    StringInterner* interner = malloc(sizeof(StringInterner));
    interner->entries = calloc(initial_capacity, sizeof(StringEntry));
    interner->count = 0;
    interner->capacity = initial_capacity;
    return interner;
}

uint32_t intern_string(StringInterner* interner, const char* str) {
    // Check if string already exists
    for (size_t i = 0; i < interner->count; i++) {
        if (strcmp(interner->entries[i].string, str) == 0) {
            return interner->entries[i].id;
        }
    }
    
    // Add new string
    if (interner->count >= interner->capacity) {
        // Expand capacity
        size_t new_capacity = interner->capacity * 2;
        StringEntry* new_entries = realloc(interner->entries, 
                                          new_capacity * sizeof(StringEntry));
        interner->entries = new_entries;
        interner->capacity = new_capacity;
    }
    
    interner->entries[interner->count].string = strdup(str);
    interner->entries[interner->count].id = interner->count;
    
    return interner->count++;
}

const char* get_interned_string(StringInterner* interner, uint32_t id) {
    if (id < interner->count) {
        return interner->entries[id].string;
    }
    return NULL;
}

void destroy_string_interner(StringInterner* interner) {
    for (size_t i = 0; i < interner->count; i++) {
        free(interner->entries[i].string);
    }
    free(interner->entries);
    free(interner);
}
```

### Pattern 8: String Views (Zero-Copy String Access)

**Use Case**: Access string data without copying.

```c
typedef struct {
    const char* data;
    size_t length;
} StringView;

StringView create_string_view(const char* str) {
    StringView view = {str, strlen(str)};
    return view;
}

StringView create_string_view_with_length(const char* str, size_t length) {
    StringView view = {str, length};
    return view;
}

int string_view_equals(StringView a, StringView b) {
    if (a.length != b.length) return 0;
    return memcmp(a.data, b.data, a.length) == 0;
}

StringView string_view_substring(StringView view, size_t start, size_t length) {
    if (start >= view.length) {
        return (StringView){NULL, 0};
    }
    if (start + length > view.length) {
        length = view.length - start;
    }
    return (StringView){view.data + start, length};
}
```

## Memory Hierarchy Optimization

### Pattern 9: L1 Cache Optimization

**Use Case**: Design data structures that fit entirely in L1 cache.

```c
// L1 cache-optimized hash table (32KB total)
#define L1_HASH_SIZE 4096  // 4K entries * 8 bytes = 32KB

typedef struct {
    uint32_t keys[L1_HASH_SIZE];
    uint32_t values[L1_HASH_SIZE];
    uint32_t mask;  // L1_HASH_SIZE - 1 for fast modulo
} L1HashTable;

L1HashTable* create_l1_hash_table() {
    L1HashTable* table = malloc(sizeof(L1HashTable));
    memset(table->keys, 0, sizeof(table->keys));
    memset(table->values, 0, sizeof(table->values));
    table->mask = L1_HASH_SIZE - 1;
    return table;
}

void l1_hash_insert(L1HashTable* table, uint32_t key, uint32_t value) {
    uint32_t hash = key;
    uint32_t index = hash & table->mask;
    
    // Linear probing
    while (table->keys[index] != 0 && table->keys[index] != key) {
        index = (index + 1) & table->mask;
    }
    
    table->keys[index] = key;
    table->values[index] = value;
}

uint32_t l1_hash_get(L1HashTable* table, uint32_t key) {
    uint32_t hash = key;
    uint32_t index = hash & table->mask;
    
    // Linear probing
    while (table->keys[index] != 0) {
        if (table->keys[index] == key) {
            return table->values[index];
        }
        index = (index + 1) & table->mask;
    }
    
    return 0;  // Not found
}
```

### Pattern 10: L2 Cache Optimization

**Use Case**: Design algorithms that work efficiently with L2 cache size.

```c
// L2 cache-optimized sorting (256KB working set)
#define L2_SORT_CHUNK_SIZE (64 * 1024)  // 64KB chunks

typedef struct {
    uint32_t* data;
    size_t size;
    uint32_t* temp_buffer;
} L2OptimizedSorter;

L2OptimizedSorter* create_l2_sorter(size_t max_size) {
    L2OptimizedSorter* sorter = malloc(sizeof(L2OptimizedSorter));
    sorter->data = malloc(max_size * sizeof(uint32_t));
    sorter->temp_buffer = malloc(L2_SORT_CHUNK_SIZE * sizeof(uint32_t));
    sorter->size = 0;
    return sorter;
}

void l2_merge_sort_chunk(uint32_t* data, size_t size, uint32_t* temp) {
    if (size <= 1) return;
    
    size_t mid = size / 2;
    l2_merge_sort_chunk(data, mid, temp);
    l2_merge_sort_chunk(data + mid, size - mid, temp);
    
    // Merge using temp buffer
    size_t i = 0, j = mid, k = 0;
    while (i < mid && j < size) {
        if (data[i] <= data[j]) {
            temp[k++] = data[i++];
        } else {
            temp[k++] = data[j++];
        }
    }
    
    while (i < mid) temp[k++] = data[i++];
    while (j < size) temp[k++] = data[j++];
    
    memcpy(data, temp, size * sizeof(uint32_t));
}

void l2_sort(L2OptimizedSorter* sorter) {
    l2_merge_sort_chunk(sorter->data, sorter->size, sorter->temp_buffer);
}
```

## Zero-Copy Patterns

### Pattern 11: Memory Mapping

**Use Case**: Access large files without copying data into memory.

```c
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct {
    void* data;
    size_t size;
    int fd;
} MemoryMappedFile;

MemoryMappedFile* mmap_file(const char* filename) {
    MemoryMappedFile* mmf = malloc(sizeof(MemoryMappedFile));
    
    mmf->fd = open(filename, O_RDONLY);
    if (mmf->fd == -1) {
        free(mmf);
        return NULL;
    }
    
    struct stat st;
    if (fstat(mmf->fd, &st) == -1) {
        close(mmf->fd);
        free(mmf);
        return NULL;
    }
    
    mmf->size = st.st_size;
    mmf->data = mmap(NULL, mmf->size, PROT_READ, MAP_PRIVATE, mmf->fd, 0);
    
    if (mmf->data == MAP_FAILED) {
        close(mmf->fd);
        free(mmf);
        return NULL;
    }
    
    return mmf;
}

void unmap_file(MemoryMappedFile* mmf) {
    munmap(mmf->data, mmf->size);
    close(mmf->fd);
    free(mmf);
}
```

### Pattern 12: Buffer Sharing

**Use Case**: Share buffers between components without copying.

```c
typedef struct {
    void* data;
    size_t size;
    size_t ref_count;
    void (*cleanup_fn)(void*);
} SharedBuffer;

SharedBuffer* create_shared_buffer(size_t size, void (*cleanup_fn)(void*)) {
    SharedBuffer* buffer = malloc(sizeof(SharedBuffer));
    buffer->data = malloc(size);
    buffer->size = size;
    buffer->ref_count = 1;
    buffer->cleanup_fn = cleanup_fn;
    return buffer;
}

SharedBuffer* share_buffer(SharedBuffer* buffer) {
    buffer->ref_count++;
    return buffer;
}

void release_buffer(SharedBuffer* buffer) {
    buffer->ref_count--;
    if (buffer->ref_count == 0) {
        if (buffer->cleanup_fn) {
            buffer->cleanup_fn(buffer->data);
        }
        free(buffer->data);
        free(buffer);
    }
}
```

## Memory Profiling Patterns

### Pattern 13: Memory Usage Tracking

**Use Case**: Track memory allocation and usage patterns.

```c
#include <stdint.h>

typedef struct {
    size_t total_allocated;
    size_t total_freed;
    size_t peak_usage;
    size_t current_usage;
    size_t allocation_count;
    size_t free_count;
} MemoryStats;

static MemoryStats g_memory_stats = {0};

void* tracked_malloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr) {
        g_memory_stats.total_allocated += size;
        g_memory_stats.allocation_count++;
        g_memory_stats.current_usage += size;
        
        if (g_memory_stats.current_usage > g_memory_stats.peak_usage) {
            g_memory_stats.peak_usage = g_memory_stats.current_usage;
        }
    }
    return ptr;
}

void tracked_free(void* ptr, size_t size) {
    free(ptr);
    g_memory_stats.total_freed += size;
    g_memory_stats.free_count++;
    g_memory_stats.current_usage -= size;
}

void print_memory_stats() {
    printf("Memory Statistics:\n");
    printf("  Total allocated: %zu bytes\n", g_memory_stats.total_allocated);
    printf("  Total freed: %zu bytes\n", g_memory_stats.total_freed);
    printf("  Current usage: %zu bytes\n", g_memory_stats.current_usage);
    printf("  Peak usage: %zu bytes\n", g_memory_stats.peak_usage);
    printf("  Allocation count: %zu\n", g_memory_stats.allocation_count);
    printf("  Free count: %zu\n", g_memory_stats.free_count);
}
```

### Pattern 14: Memory Leak Detection

**Use Case**: Detect memory leaks in development and testing.

```c
typedef struct {
    void* ptr;
    size_t size;
    const char* file;
    int line;
    const char* function;
} AllocationRecord;

typedef struct {
    AllocationRecord* records;
    size_t count;
    size_t capacity;
} LeakDetector;

static LeakDetector g_leak_detector = {0};

void* leak_detected_malloc(size_t size, const char* file, int line, const char* function) {
    void* ptr = malloc(size);
    if (ptr) {
        // Add allocation record
        if (g_leak_detector.count >= g_leak_detector.capacity) {
            size_t new_capacity = g_leak_detector.capacity * 2 + 1;
            AllocationRecord* new_records = realloc(g_leak_detector.records,
                                                   new_capacity * sizeof(AllocationRecord));
            g_leak_detector.records = new_records;
            g_leak_detector.capacity = new_capacity;
        }
        
        g_leak_detector.records[g_leak_detector.count].ptr = ptr;
        g_leak_detector.records[g_leak_detector.count].size = size;
        g_leak_detector.records[g_leak_detector.count].file = file;
        g_leak_detector.records[g_leak_detector.count].line = line;
        g_leak_detector.records[g_leak_detector.count].function = function;
        g_leak_detector.count++;
    }
    return ptr;
}

void leak_detected_free(void* ptr) {
    // Remove allocation record
    for (size_t i = 0; i < g_leak_detector.count; i++) {
        if (g_leak_detector.records[i].ptr == ptr) {
            // Remove record by shifting
            for (size_t j = i; j < g_leak_detector.count - 1; j++) {
                g_leak_detector.records[j] = g_leak_detector.records[j + 1];
            }
            g_leak_detector.count--;
            break;
        }
    }
    free(ptr);
}

void print_memory_leaks() {
    printf("Memory Leaks Detected: %zu\n", g_leak_detector.count);
    for (size_t i = 0; i < g_leak_detector.count; i++) {
        AllocationRecord* record = &g_leak_detector.records[i];
        printf("  Leak %zu: %p (%zu bytes) at %s:%d in %s\n",
               i, record->ptr, record->size, record->file, record->line, record->function);
    }
}

// Macros for easy usage
#define LEAK_MALLOC(size) leak_detected_malloc(size, __FILE__, __LINE__, __FUNCTION__)
#define LEAK_FREE(ptr) leak_detected_free(ptr)
```

## Memory Optimization Best Practices

### 1. Cache-Aware Design
- Keep hot data in L1 cache (< 32KB)
- Design algorithms for L2 cache (< 256KB)
- Minimize cache misses

### 2. Allocation Strategy
- Use stack allocation for small, short-lived objects
- Use memory pools for fixed-size objects
- Use arenas for related allocations

### 3. Zero-Copy Operations
- Use string views instead of copying
- Share buffers between components
- Use memory mapping for large files

### 4. Memory Profiling
- Track allocation patterns in development
- Monitor memory usage in production
- Detect and fix memory leaks

### 5. Performance Monitoring
- Measure cache hit rates
- Monitor memory bandwidth usage
- Profile memory access patterns

## Conclusion

The memory optimization patterns in this cookbook provide comprehensive strategies for achieving **7-tick performance** while maintaining efficient memory usage. Key takeaways:

1. **Cache-aware design**: Optimize for L1/L2/L3 cache hierarchies
2. **Efficient allocation**: Use appropriate allocation strategies for different use cases
3. **Zero-copy operations**: Minimize memory copying overhead
4. **Memory pooling**: Reuse objects and pre-allocate memory
5. **String optimization**: Use interning and views for efficient string handling
6. **Memory profiling**: Track and optimize memory usage patterns

These patterns ensure that the 7T engine achieves optimal performance while maintaining efficient memory management across all components. 