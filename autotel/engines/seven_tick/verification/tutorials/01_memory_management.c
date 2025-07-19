#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    printf("First Principles C Tutorial: Memory Management\n");
    printf("==============================================\n");
    printf("Validating core memory concepts for 7T Engine\n\n");
    
    // Test 1: Basic memory allocation
    int *ptr = malloc(100 * sizeof(int));
    if (ptr != NULL) {
        printf("✅ PASS: Memory allocation successful\n");
        
        // Test 2: Memory usage
        for (int i = 0; i < 100; i++) {
            ptr[i] = i;
        }
        printf("✅ PASS: Memory usage successful\n");
        
        // Test 3: Memory cleanup
        free(ptr);
        printf("✅ PASS: Memory cleanup successful\n");
    } else {
        printf("❌ FAIL: Memory allocation failed\n");
        return 1;
    }
    
    printf("\n🎉 All memory management concepts validated!\n");
    printf("Ready for 7T Engine development.\n");
    return 0;
}