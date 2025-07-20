#include "cns/8t/interfaces.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// 8T Cache Implementation - 80/20 LRU cache

typedef struct cache_node {
    cache_entry_t entry;
    struct cache_node* next;
    struct cache_node* prev;
} cache_node_t;

typedef struct {
    cache_node_t* head;
    cache_node_t* tail;
    cache_node_t** buckets;
    size_t capacity;
    size_t size;
    size_t bucket_count;
    pthread_rwlock_t lock;
} cache_context_t;

static cache_context_t* g_cache_ctx = NULL;

static uint32_t hash_key(uint64_t key, size_t bucket_count) {
    // Simple hash function
    return (uint32_t)(key % bucket_count);
}

static int cache_init(size_t capacity) {
    if (g_cache_ctx != NULL) {
        return -1;
    }
    
    g_cache_ctx = calloc(1, sizeof(cache_context_t));
    if (g_cache_ctx == NULL) {
        return -1;
    }
    
    g_cache_ctx->capacity = capacity;
    g_cache_ctx->bucket_count = capacity / 4; // 80/20: 4 items per bucket avg
    g_cache_ctx->buckets = calloc(g_cache_ctx->bucket_count, sizeof(cache_node_t*));
    
    if (g_cache_ctx->buckets == NULL) {
        free(g_cache_ctx);
        g_cache_ctx = NULL;
        return -1;
    }
    
    pthread_rwlock_init(&g_cache_ctx->lock, NULL);
    return 0;
}

static void* cache_get(uint64_t key) {
    if (g_cache_ctx == NULL) {
        return NULL;
    }
    
    pthread_rwlock_rdlock(&g_cache_ctx->lock);
    
    uint32_t bucket = hash_key(key, g_cache_ctx->bucket_count);
    cache_node_t* node = g_cache_ctx->buckets[bucket];
    
    while (node != NULL) {
        if (node->entry.key == key) {
            node->entry.access_count++;
            void* value = node->entry.value;
            pthread_rwlock_unlock(&g_cache_ctx->lock);
            return value;
        }
        node = node->next;
    }
    
    pthread_rwlock_unlock(&g_cache_ctx->lock);
    return NULL;
}

static int cache_put(uint64_t key, void* value) {
    if (g_cache_ctx == NULL) {
        return -1;
    }
    
    pthread_rwlock_wrlock(&g_cache_ctx->lock);
    
    // Check if key exists
    uint32_t bucket = hash_key(key, g_cache_ctx->bucket_count);
    cache_node_t* node = g_cache_ctx->buckets[bucket];
    
    while (node != NULL) {
        if (node->entry.key == key) {
            node->entry.value = value;
            node->entry.access_count++;
            pthread_rwlock_unlock(&g_cache_ctx->lock);
            return 0;
        }
        node = node->next;
    }
    
    // Add new entry
    if (g_cache_ctx->size >= g_cache_ctx->capacity) {
        // 80/20: Simple eviction - remove from tail
        cache_node_t* evict = g_cache_ctx->tail;
        if (evict != NULL) {
            if (evict->prev) {
                evict->prev->next = NULL;
            }
            g_cache_ctx->tail = evict->prev;
            
            // Remove from bucket
            uint32_t evict_bucket = hash_key(evict->entry.key, g_cache_ctx->bucket_count);
            cache_node_t** bucket_ptr = &g_cache_ctx->buckets[evict_bucket];
            while (*bucket_ptr != NULL) {
                if (*bucket_ptr == evict) {
                    *bucket_ptr = evict->next;
                    break;
                }
                bucket_ptr = &(*bucket_ptr)->next;
            }
            
            free(evict);
            g_cache_ctx->size--;
        }
    }
    
    // Create new node
    cache_node_t* new_node = calloc(1, sizeof(cache_node_t));
    if (new_node == NULL) {
        pthread_rwlock_unlock(&g_cache_ctx->lock);
        return -1;
    }
    
    new_node->entry.key = key;
    new_node->entry.value = value;
    new_node->entry.access_count = 1;
    
    // Add to bucket
    new_node->next = g_cache_ctx->buckets[bucket];
    g_cache_ctx->buckets[bucket] = new_node;
    
    // Add to head of LRU list
    if (g_cache_ctx->head == NULL) {
        g_cache_ctx->head = g_cache_ctx->tail = new_node;
    } else {
        new_node->next = g_cache_ctx->head;
        g_cache_ctx->head->prev = new_node;
        g_cache_ctx->head = new_node;
    }
    
    g_cache_ctx->size++;
    pthread_rwlock_unlock(&g_cache_ctx->lock);
    return 0;
}

static void cache_clear(void) {
    if (g_cache_ctx == NULL) {
        return;
    }
    
    pthread_rwlock_wrlock(&g_cache_ctx->lock);
    
    // Free all nodes
    cache_node_t* node = g_cache_ctx->head;
    while (node != NULL) {
        cache_node_t* next = node->next;
        free(node);
        node = next;
    }
    
    // Clear buckets
    memset(g_cache_ctx->buckets, 0, g_cache_ctx->bucket_count * sizeof(cache_node_t*));
    g_cache_ctx->head = g_cache_ctx->tail = NULL;
    g_cache_ctx->size = 0;
    
    pthread_rwlock_unlock(&g_cache_ctx->lock);
}

static void cache_cleanup(void) {
    if (g_cache_ctx != NULL) {
        cache_clear();
        pthread_rwlock_destroy(&g_cache_ctx->lock);
        free(g_cache_ctx->buckets);
        free(g_cache_ctx);
        g_cache_ctx = NULL;
    }
}

// Export cache interface
static cache_interface_t t8_cache = {
    .init = cache_init,
    .get = cache_get,
    .put = cache_put,
    .clear = cache_clear,
    .cleanup = cache_cleanup
};

cache_interface_t* t8_get_cache(void) {
    return &t8_cache;
}