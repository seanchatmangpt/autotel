#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cjinja_ultra_portable.h"

int main() {
    printf("Debugging ultra-fast implementation...\n");
    
    CJinjaUltraContext* ctx = cjinja_ultra_create_context();
    if (!ctx) {
        printf("Failed to create context\n");
        return 1;
    }
    
    printf("Setting variable: name = Alice\n");
    cjinja_ultra_set_var(ctx, "name", "Alice");
    
    printf("Retrieving variable: name\n");
    const char* name = cjinja_ultra_get_var(ctx, "name");
    
    printf("Retrieved value: '%s'\n", name ? name : "NULL");
    printf("Pool used: %zu\n", ctx->pool_used);
    printf("Total variables: %zu\n", ctx->total_variables);
    
    // Check if the variable is in the hash table
    uint32_t hash = cjinja_ultra_hash("name", 4);
    uint32_t bucket = hash & 255;
    printf("Hash: %u, Bucket: %u\n", hash, bucket);
    
    CJinjaHashEntry* entry = ctx->buckets[bucket];
    if (entry) {
        printf("Found entry in bucket:\n");
        printf("  Key: '%s'\n", entry->key);
        printf("  Value: '%s'\n", entry->value);
        printf("  Hash: %u\n", entry->key_hash);
        printf("  Key len: %u\n", entry->key_len);
    } else {
        printf("No entry found in bucket %u\n", bucket);
    }
    
    cjinja_ultra_destroy_context(ctx);
    return 0;
}